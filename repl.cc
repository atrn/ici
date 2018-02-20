#define ICI_CORE

#include "repl.h"

#include "file.h"
#include "ftype.h"
#include "str.h"
#include "map.h"
#include "array.h"

#include <ctype.h>
#include <unistd.h>

namespace ici {

struct repl_file {
    ref<file> in_;
    ref<file> out_;
    ref<str> prompt_;
    bool emitprompt_ = true;
    bool eof_ = false;
    bool sol_ = true;
    bool interactive_ = true;

    repl_file()
        : in_(make_ref(need_stdin(), with_incref))
        , out_(make_ref(need_stdout(), with_incref))
        , prompt_(make_ref(SS(replprompt), with_incref))
        , interactive_(isatty(0))
    {
    }

    void needprompt() {
        emitprompt_ = true;
    }

    void reset() {
        needprompt();
    }

    int write(const void *s, int n) {
        return out_->write(s, n);
    }

    void puts(const char *s) {
        write(s, strlen(s));
    }

    void command() {
        char buf[1024+1];
        char *line = buf + 1;
        char *p = line;

        for (;;) {
            auto ch = in_->getch();
            if (ch == '\n' || ch == EOF) {
                *p = '\0';
                break;
            }
            *p++ = ch;
            if ((p - line) == (sizeof buf - 2)) {
                p[-1] = '\0';
                break;
            }
        }

        if (p - line < 1) {
            return;
        }

        if (line[0] == 'q' && !line[1]) {
            exit(0);
        }

        if (line[0] == 'h' && !line[1]) {
            puts
            (
                "Commands:\n"
                "    .h         get help (this message)\n"
                "    .r <path>  parse file\n"
                "    .p <expr>  print an expression\n"
                "    .q         quit, exit the interpreter\n"
            );
            return;
        }

        if (line[0] == 'r') {
            for (p = line+1; *p && isspace(*p); ++p) {
                ;
            }
            if (!*p) {
                puts(".r requires a <pathname> parameter\n");
                return;
            }
            if (parse_file(p)) {
                puts(".r ");
                puts(error);
                puts("\n");
                clear_error();
            }
            return;
        }

        if (line[0] == 'p' && isspace(line[1])) {
            for (p = line+1; *p && isspace(*p); ++p) {
                ;
            }
            if (!*p) {
                puts(".p requires an expression parameter\n");
                return;
            }
            // replace the "p " at the start of the line to
            // assign the expression to the variable _.
            //
            line[-1] = '_';
            line[0] = ':';
            line[1] = '=';
            for (p = line+2; *p; ++p) {
            }
            if ((p - buf) < ptrdiff_t(sizeof buf - 2)) {
                p[0] = ';';
                p[1] = '\0';
            } else {
                // should fail
            }
            ref<file> f = sopen(buf, p-buf, nullptr);
            if (!f) {
                return;
            }
            auto scope = vs.a_top[-1];
            if (parse_file(f, objwsupof(scope))) {
                puts(error);
                puts("\n");
            }
            if (auto result = ici_fetch(scope, SS(_))) {
                call(SS(println), "o", result);
            }
            return;
        }

        puts(".");
        puts(line);
        puts(" not recognized\n");
    }

}; // class repl_file

void repl_file_new_statement(void *fp) {
    auto rf = static_cast<repl_file *>(fp);
    if (rf->sol_) {
        rf->needprompt();
    }
}

// file pointer is a repl_file *
class repl_ftype : public stdio_ftype {
public:
    repl_ftype() {
    }

    void emitprompt(repl_file *rf) {
        if (rf->emitprompt_ && rf->interactive_) {
            rf->write(rf->prompt_->s_chars, rf->prompt_->s_nchars);
        }
        rf->emitprompt_ = false;
    }

    int getch(void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        emitprompt(rf);
        for (;;) {
            auto ch = rf->in_->getch();
            if (ch == EOF) {
                rf->eof_ = true;
                return ch;
            }
            if (rf->sol_ && ch == '.') {
                rf->command();
                rf->needprompt();
                emitprompt(rf);
            } else {
                rf->sol_ = ch == '\n';
                return ch;
            }
        }
    }

    int ungetch(int ch, void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        return rf->in_->ungetch(ch);
    }
    
    int close(void *) override {
        return 0;
    }

    long seek(void *, long, int) override {
        errno = ESPIPE;
        return -1;
    }

    int eof(void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        return rf->eof_;
    }

    int read(void *buf, long n, void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        emitprompt(rf);
        return rf->in_->read(buf, n);
    }

    int write(const void *buf, long n, void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        auto result = rf->write(buf, n);
        rf->needprompt();
        return result;
    }

    int fileno(void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        return rf->in_->fileno();
    }

    int setvbuf(void *fp, char *p, int f, size_t z) override {
        auto rf = static_cast<repl_file *>(fp);
        return rf->out_->setvbuf(p, f, z);
    }
};

class repl_ftype *repl_ftype = singleton<class repl_ftype>();

bool is_repl_file(file *file) {
    return file->f_type == repl_ftype;
}

void repl() {
    auto failed = [](const char *why = error) -> void {
        fprintf(stderr, "\nerror: %s\n", why);
    };

    //  We get called before ici::main creates argc/argv so we create
    //  a fake pair for code that may want them.
    //
    ref<array> av = new_array(1);
    av->push_back(SS(repl));
    int64_t l = 1;
    if (set_val(objwsupof(vs.a_top[-1])->o_super, SS(argv), 'o', av)) {
        return failed();
    }
    if (set_val(objwsupof(vs.a_top[-1])->o_super, SS(argc), 'i', &l)) {
        return failed();
    }

    //  The repl_file takes over stdin and stdout so it can track
    //  input and output and know when to output prompts.
    //
    repl_file repl;

    ref<file> file = new_file(&repl, repl_ftype, SS(repl), nullptr);
    if (!file) {
        return failed();
    }

    ref<objwsup> a;
    ref<objwsup> s;

    //  Create the repl's scope.
    //
    if ((a = objwsupof(new_map())) == nullptr) {
        return failed();
    }
    if ((s = objwsupof(new_map())) == nullptr) {
        return failed();
    }
    s->o_super = objwsupof(vs.a_top[-1])->o_super;
    a->o_super = s.release(with_decref);

    //  Replace stdout with our file so it can track output.
    //
    if (ici_assign(vs.a_top[-1], SS(_stdout), file)) {
        return failed();
    }

    while (!repl.eof_) {
        repl.reset();
        parse_file(file, a);
        if (error != nullptr) {
            repl.puts("error: ");
            repl.write(error, strlen(error));
            repl.puts("\n");
        }
    }

    if (repl.interactive_) {
        repl.puts("\n");
    }
}

} // namespace ici
