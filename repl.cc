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

namespace {

FILE *repl_log = nullptr;

object *current_scope() {
    return vs.a_top[-1];
}

objwsup *current_locals() {
    return objwsupof(current_scope())->o_super;
}

}

struct repl_file {
    ref<file> stdin_;
    ref<file> stdout_;
    ref<str> prompt_ = make_ref(SS(replprompt), with_incref);
    bool emitprompt_ = true;
    bool extra_ = false;
    bool eof_ = false;
    bool sol_ = true;
    bool p_ = false;
    bool interactive_ = false;

    repl_file()
        : stdin_(make_ref(need_stdin(), with_incref))
        , stdout_(make_ref(need_stdout(), with_incref))
        , interactive_(isatty(fileno((FILE *)stdin_->f_file)))
    {
    }

    void needprompt() {
        if (repl_log) { fprintf(repl_log, "needprompt\n"); }
        emitprompt_ = true;
        p_ = false;
    }

    void reset() {
        if (repl_log) { fprintf(repl_log, "reset\n"); }
        emitprompt_ = true;
        sol_ = true;
        p_ = false;
    }

    int write(const void *s, int n) {
        return stdout_->write(s, n);
    }

    void puts(const char *s) {
        write(s, strlen(s));
    }

    void puts(const str *s) {
        write(s->s_chars, s->s_nchars);
    }

    void command() {
        if (repl_log) {fprintf(repl_log, "command\n");}
        char buf[1024+1];
        char *line = buf + 1;
        char *p = line;

        for (;;) {
            auto ch = stdin_->getch();
            if (ch == '\n' || ch == EOF) {
                *p = '\0';
                break;
            }
            *p++ = ch;
            if ((p - line) == (sizeof buf - 2)) {
                *--p = '\0';
                break;
            }
        }

        if (p - line < 1) {
            return;
        }

        // .q
        if (line[0] == 'q' && !line[1]) {
            exit(0);
        }

        // .h
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

        // .r ...
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

        // .p ...
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
                // should abort
            }
            ref<file> f = sopen(buf, p-buf, nullptr);
            if (!f) {
                return;
            }
            if (parse_file(f, objwsupof(current_scope()))) {
                puts(error);
                puts("\n");
            }
            if (auto result = ici_fetch(current_scope(), SS(_))) {
                call(SS(println), "o", result);
            }
            return;
        }

        // unrecognized
        puts(".");
        puts(line);
        puts(" not recognized\n");
    }

}; // class repl_file

void repl_file_new_statement(void *fp, bool top) {
    auto rf = static_cast<repl_file *>(fp);
    if (repl_log) { fprintf(repl_log, "repl_file_new_statement: top=%s rf->sol_=%s\n", (top ? "true":"false"), (rf->sol_ ? "true":"false")); }
    if (top) {
        rf->extra_ = false;
        rf->needprompt();
    } else {
        rf->extra_ = true;
    }
}

// file pointer is a repl_file *
class repl_ftype : public stdio_ftype {
public:
    void emitprompt(repl_file *rf) {
        const char * const promptchars = ">> ";
        if (!rf->interactive_ || rf->p_) {
            return;
        }
        if (rf->emitprompt_ || rf->sol_) {
            if (repl_log) { fprintf(repl_log, "emitprompt rf->p_=%d rf->emitprompt=%d rf->sol_=%d\n", rf->p_, rf->emitprompt_, rf->sol_); }
            rf->puts(rf->prompt_);
            rf->puts(rf->extra_ ? promptchars : promptchars + 1);
            rf->p_ = true;
        }
        rf->emitprompt_ = false;
    }

    int getch(void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        if (repl_log) {fprintf(repl_log, "getch: p_=%d sol_%d extra_%d\n", rf->p_, rf->sol_, rf->extra_); }
        emitprompt(rf);
        for (;;) {
            auto ch = rf->stdin_->getch();
            if (ch == EOF) {
                rf->eof_ = true;
                return ch;
            }
            if (rf->sol_ && ch == '.') {
                rf->command();
                rf->needprompt();
                emitprompt(rf);
                continue;
            }
            if (ch == '\n') {
                rf->p_ = !rf->sol_ && !rf->extra_;
                rf->sol_ = true;
            } else {
                rf->sol_ = false;
                rf->p_ = false;
            }
            return ch;
        }
    }

    int ungetch(int ch, void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        if (repl_log) {fprintf(repl_log, "ungetch\n"); }
        return rf->stdin_->ungetch(ch);
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
        return rf->stdin_->read(buf, n);
    }

    int write(const void *buf, long n, void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        if (repl_log) { fprintf(repl_log, "write: rf->sol_=%s\n", (rf->sol_ ? "true":"false")); }
        auto result = rf->write(buf, n);
        if (rf->sol_) {
            rf->needprompt();
        }
        return result;
    }

    int fileno(void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        return rf->stdin_->fileno();
    }

    int setvbuf(void *fp, char *p, int f, size_t z) override {
        auto rf = static_cast<repl_file *>(fp);
        return rf->stdout_->setvbuf(p, f, z);
    }
};

class repl_ftype *repl_ftype = singleton<class repl_ftype>();

bool is_repl_file(file *file) {
    return file->f_type == repl_ftype;
}

namespace {

struct set_std_file {
    ref<str> name_;
    file *orig_;
    bool undo_;

    set_std_file(str *name, file *orig, file *newf)
        : name_(make_ref(name, with_incref))
        , orig_(orig)
        , undo_(ici_assign(current_scope(), name_, newf) == 0)
    {
    }

    ~set_std_file() {
        if (undo_) {
            ici_assign(current_scope(), name_, orig_);
        }
    }

    set_std_file(const set_std_file &) = delete;
    set_std_file& operator=(const set_std_file &) = delete;
};

} // anon

void repl() {
    // repl_log = fopen("repl.log", "w");

    auto failed = [](const char *why = error) -> void {
        fprintf(stderr, "\nerror: %s\n", why);
    };

    //  We get called before ici::main creates argc/argv so we create
    //  them in case code wants to look at them.
    //
    ref<array> argv = new_array(1);
    argv->push_back(SS(repl));
    int64_t argc = 1;
    if (set_val(current_locals(), SS(argv), 'o', argv)) {
        return failed();
    }
    if (set_val(current_locals(), SS(argc), 'i', &argc)) {
        return failed();
    }

    //  Make a scope for our parsing.
    //
    auto locals = make_ref<objwsup>(new_map(current_locals()));
    if (!locals) {
        return failed();
    }
    auto scope = make_ref<objwsup>(new_map(locals.release(with_decref)));
    if (!scope) {
        return failed();
    }

    //  The repl_file instance takes over stdin and stdout so it can
    //  track input and output and know when to output prompts.
    //
    repl_file repl;
    ref<file> file = new_file(&repl, repl_ftype, SS(repl), nullptr);
    if (!file) {
        return failed();
    }

    set_std_file set_stdin(SS(_stdin), repl.stdin_, file);
    set_std_file set_stdout(SS(_stdout), repl.stdout_, file);

    while (!repl.eof_) {
        repl.reset();
        if (parse_file(file, scope)) {
            repl.puts("error: ");
            repl.puts(error);
            repl.puts("\n");
        }
    }
    if (repl.interactive_) {
        repl.puts("\n");
    }
    if (repl_log) fclose(repl_log);
}

} // namespace ici
