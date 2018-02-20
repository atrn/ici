#define ICI_CORE

#include "repl.h"

#include "file.h"
#include "ftype.h"
#include "str.h"
#include "map.h"
#include "array.h"

namespace ici {

struct repl_file {
    file *in_;
    file *out_;
    ref<str> prompt_;
    bool emitprompt_ = true;
    bool eof_ = false;
    bool sol_ = true;

    void needprompt() {
        emitprompt_ = true;
    }

    void reset() {
        needprompt();
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
    fprintf(stderr, "REPL: command \"%s\"\n", line);
}

void repl_file_new_statement(void *fp) {
    auto rf = static_cast<repl_file *>(fp);
    rf->needprompt();
}

// file pointer is a repl_file *
class repl_ftype : public stdio_ftype {
    void emitprompt(repl_file *rf) {
        rf->out_->write(rf->prompt_->s_chars, rf->prompt_->s_nchars);
        rf->emitprompt_ = false;
    }

public:
    int getch(void *fp) override {
        auto rf = static_cast<repl_file *>(fp);
        if (rf->emitprompt_) {
            emitprompt(rf);
        }
    again:
        auto ch = rf->in_->getch();
        if (ch == EOF) {
            rf->eof_ = true;
        }
        if (rf->sol_ && ch == '.') {
            repl_command(rf);
            emitprompt(rf);
            goto again;
        }
        rf->sol_ = ch == '\n';
        return ch;
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
        auto result = rf->out_->write(buf, n);
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

ftype *repl_ftype = singleton<class repl_ftype>();

void repl() {
    auto failed = [](const char *why) -> void {
        fprintf(stderr, "\nERROR: %s\n", why);
    };

    auto fp = repl_file{
        need_stdin(),
        need_stdout(),
        str_get_nul_term("ici> "),
    };

    ref<file> f = new_file(&fp, repl_ftype, SS(empty_string), nullptr);
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
    if (ici_assign(vs.a_top[-1], SS(_stdout), f)) {
        return failed(error);
    }
    while (!fp.eof_) {
        fp.reset();
        parse_file(f, a);
        if (error != nullptr) {
            fp.out_->write("error: ", 7);
            fp.out_->write(error, strlen(error));
            fp.out_->write("\n", 1);
        }
    }
    fp.out_->write("\n", 1);
}

} // namespace ici
