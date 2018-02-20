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
    str *prompt_;
    bool emitprompt_ = true;
    bool eof_ = false;
    bool sol_ = true;

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
};

void repl_command(repl_file *rf) {
    char line[1024];
    char *p = line;

    for (;;) {
        auto ch = rf->in_->getch();
        if (ch == '\n' || ch == EOF) {
            *p = '\0';
            break;
        }
        *p++ = ch;
        if ((p - line) == sizeof line) {
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

    if (line[0] == '?' && !line[1]) {
        rf->puts
        (
            "Commands:\n"
            "    .?         get help (this message)\n"
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
            rf->puts(".r requires a <pathname> parameter\n");
            return;
        }
        if (parse_file(p)) {
            rf->puts(".r ");
            rf->puts(error);
            rf->puts("\n");
            clear_error();
        }
        return;
    }

    if (line[0] == 'p') {
        if (!line[1]) {
            rf->puts(".p requires an expression parameter\n");
            return;
        }
        if (isspace(line[1])) {
            return;
        }
    }

    rf->puts(".");
    rf->puts(line);
    rf->puts(": common not recognized\n");
}

void repl_file_new_statement(void *fp) {
    auto rf = static_cast<repl_file *>(fp);
    rf->needprompt();
}

// file pointer is a repl_file *
class repl_filetype : public stdio_ftype {
public:
    repl_filetype() {
        interactive = isatty(0);
    }

    void emitprompt(repl_file *rf) {
        if (interactive) {
            rf->write(rf->prompt_->s_chars, rf->prompt_->s_nchars);
            rf->emitprompt_ = false;
        }
    }

    bool interactive;

    int getch(void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        if (rf->emitprompt_) {
            emitprompt(rf);
        }
        for (;;) {
            auto ch = rf->in_->getch();
            if (ch == EOF) {
                rf->eof_ = true;
                return ch;
            }
            if (rf->sol_ && ch == '.') {
                repl_command(rf);
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
        if (rf->emitprompt_) {
            emitprompt(rf);
        }
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

ftype *repl_ftype = singleton<repl_filetype>();

void repl() {
    ref<array> av = new_array(1);
    av->push_back(SS(repl));
    int64_t l = 1;
    set_val(objwsupof(vs.a_top[-1])->o_super, SS(argv), 'o', av);
    set_val(objwsupof(vs.a_top[-1])->o_super, SS(argc), 'i', &l);

    auto failed = [](const char *why) -> void {
        fprintf(stderr, "\nerror: %s\n", why);
    };

    repl_file rf{need_stdin(), need_stdout(), SS(replprompt)};
    ref<file> f = new_file(&rf, repl_ftype, SS(repl), nullptr);
    if (!f) {
        return failed(error);
    }

    ref<objwsup> a;
    objwsup *s;

    if ((a = objwsupof(new_map())) == nullptr) {
        return failed(error);
    }
    if (ici_assign(a, SS(_file_), f->f_name)) {
        return failed(error);
    }
    if ((a->o_super = s = objwsupof(new_map())) == nullptr) {
        return failed(error);
    }
    decref(s);
    s->o_super = objwsupof(vs.a_top[-1])->o_super;
    need_stdout()->incref(); // to account for our ref in rf
    if (ici_assign(vs.a_top[-1], SS(_stdout), f)) {
        return failed(error);
    }
    while (!rf.eof_) {
        rf.reset();
        parse_file(f, a);
        if (error != nullptr) {
            rf.puts("error: ");
            rf.write(error, strlen(error));
            rf.puts("\n");
        }
    }
    if (((repl_filetype *)repl_ftype)->interactive) {
        rf.puts("\n");
    }
}

} // namespace ici
