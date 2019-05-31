// -*- mode:c++ -*-

/*
 * Any strings listed in this file may be refered to with SS(name) for
 * a (str *) Maximum 15 chars per string. Users need to include
 * str.h. Wherever possible, the name and the string should be the
 * same.
 *
 * String objects made by this mechanism reside in static initialised
 * memory. They are not allocated or collected, but in other respects
 * are normal.
 *
 * This file is included with variying definitions of SSTRING()
 * from str.h (for global extern defines), and twice from sstring.c.
 *
 * NB: The declaraction of the static string structur in str.h must
 * have an s_chars[] field of large enough size to cover the longest
 * string in this file.
 */
SSTRING(_, "_")
SSTRING(_space_, " ")
SSTRING(_NULL_, "NULL")
SSTRING(_close, "_close")
SSTRING(_exit, "_exit")
SSTRING(_false_, "false")
SSTRING(_file_, "_file_")
SSTRING(_func_, "_func_")
SSTRING(_popen, "popen")
SSTRING(_stderr, "stderr")
SSTRING(_stdin, "stdin")
SSTRING(_stdout, "stdout")
SSTRING(_true_, "true")
SSTRING(abs, "abs")
SSTRING(accept, "accept")
SSTRING(access, "access")
SSTRING(acct, "acct")
SSTRING(acos, "acos")
SSTRING(active, "active")
SSTRING(addr, "addr")
SSTRING(alarm, "alarm")
SSTRING(alloc, "alloc")
SSTRING(alt, "alt")
SSTRING(apply, "apply")
SSTRING(argc, "argc")
SSTRING(argcount, "argcount")
SSTRING(argerror, "argerror")
SSTRING(args, "args")
SSTRING(argv, "argv")
SSTRING(array, "array")
SSTRING(asin, "asin")
SSTRING(assign, "assign")
SSTRING(atan, "atan")
SSTRING(atan2, "atan2")
SSTRING(atime, "atime")
SSTRING(basename, "basename")
SSTRING(bind, "bind")
SSTRING(blksize, "blksize")
SSTRING(blocks, "blocks")
SSTRING(break, "break")
SSTRING(build, "build")
SSTRING(calendar, "calendar")
SSTRING(call, "call")
SSTRING(callable, "callable")
SSTRING(case, "case")
SSTRING(ceil, "ceil")
SSTRING(channel, "channel")
SSTRING(chdir, "chdir")
SSTRING(chmod, "chmod")
SSTRING(chown, "chown")
SSTRING(chroot, "chroot")
SSTRING(class, "class")
SSTRING(clock, "clock")
SSTRING(close, "close")
SSTRING(closesocket, "closesocket")
SSTRING(closure, "closure")
SSTRING(cmp, "cmp")
SSTRING(connect, "connect")
SSTRING(continue, "continue")
SSTRING(copy, "copy")
SSTRING(core, "core")
SSTRING(core1, "core1")
SSTRING(core2, "core2")
SSTRING(core3, "core3")
SSTRING(core4, "core4")
SSTRING(core5, "core5")
SSTRING(core6, "core6")
SSTRING(core7, "core7")
SSTRING(core8, "core8")
SSTRING(core9, "core9")
SSTRING(cos, "cos")
SSTRING(cpu, "cpu")
SSTRING(cputime, "cputime")
SSTRING(creat, "creat")
SSTRING(critsect, "critsect")
SSTRING(ctime, "ctime")
SSTRING(cur, "cur")
SSTRING(currentfile, "currentfile")
SSTRING(data, "data")
SSTRING(day, "day")
SSTRING(debug, "debug")
SSTRING(deepatom, "deepatom")
SSTRING(deepcopy, "deepcopy")
SSTRING(default, "default")
SSTRING(del, "del")
SSTRING(dev, "dev")
SSTRING(dir, "dir")
SSTRING(dirname, "dirname")
SSTRING(do, "do")
SSTRING(dup, "dup")
SSTRING(dupfd, "dupfd")
SSTRING(else, "else")
SSTRING(empty_string, "")
SSTRING(eof, "eof")
SSTRING(eq, "eq")
SSTRING(_error, "error")
SSTRING(errorfile, "errorfile")
SSTRING(errorline, "errorline")
SSTRING(eventloop, "eventloop")
SSTRING(except, "except")
SSTRING(exec, "exec")
SSTRING(execp, "execp")
SSTRING(exit, "exit")
SSTRING(exp, "exp")
SSTRING(explode, "explode")
SSTRING(extern, "extern")
SSTRING(fail, "fail")
SSTRING(failed, "failed")
SSTRING(fcntl, "fcntl")
SSTRING(fdopen, "fdopen")
SSTRING(fetch, "fetch")
SSTRING(fileno, "fileno")
SSTRING(finished, "finished")
SSTRING(float, "float")
SSTRING(flock, "flock")
SSTRING(floor, "floor")
SSTRING(flush, "flush")
SSTRING(fmod, "fmod")
SSTRING(fopen, "fopen")
SSTRING(for, "for")
SSTRING(forall, "forall")
SSTRING(format_time, "format_time")
SSTRING(fork, "fork")
SSTRING(fsize, "fsize")
SSTRING(func, "func")
SSTRING(gecos, "gecos")
SSTRING(get, "get")
SSTRING(getchar, "getchar")
SSTRING(getcwd, "getcwd")
SSTRING(getegid, "getegid")
SSTRING(getenv, "getenv")
SSTRING(geteuid, "geteuid")
SSTRING(getfd, "getfd")
SSTRING(getfile, "getfile")
SSTRING(getfl, "getfl")
SSTRING(getgid, "getgid")
SSTRING(gethostbyaddr, "gethostbyaddr")
SSTRING(gethostbyname, "gethostbyname")
SSTRING(getitimer, "getitimer")
SSTRING(getline, "getline")
SSTRING(getown, "getown")
SSTRING(getpass, "getpass")
SSTRING(getpeername, "getpeername")
SSTRING(getpgrp, "getpgrp")
SSTRING(getpid, "getpid")
SSTRING(getportno, "getportno")
SSTRING(getppid, "getppid")
SSTRING(getrlimit, "getrlimit")
SSTRING(getsockname, "getsockname")
SSTRING(getsockopt, "getsockopt")
SSTRING(gettimeofday, "gettimeofday")
SSTRING(gettoken, "gettoken")
SSTRING(gettokens, "gettokens")
SSTRING(getuid, "getuid")
SSTRING(gid, "gid")
SSTRING(gmtoff, "gmtoff")
SSTRING(go, "go")
SSTRING(gsub, "gsub")
SSTRING(hostname, "hostname")
SSTRING(hour, "hour")
SSTRING(_ici, "ici")
SSTRING(if, "if")
SSTRING(ignore, "ignore")
SSTRING(implode, "implode")
SSTRING(in, "in")
SSTRING(include, "include")
SSTRING(infinity, "infinity")
SSTRING(ino, "ino")
SSTRING(int, "int")
SSTRING(interval, "interval")
SSTRING(isa, "isa")
SSTRING(isatom, "isatom")
SSTRING(isatty, "isatty")
SSTRING(isdst, "isdst")
SSTRING(join, "join")
SSTRING(keys, "keys")
SSTRING(kill, "kill")
SSTRING(len, "len")
SSTRING(line, "line")
SSTRING(link, "link")
SSTRING(listen, "listen")
SSTRING(load, "load")
SSTRING(local, "local")
SSTRING(lockf, "lockf")
SSTRING(log, "log")
SSTRING(log10, "log10")
SSTRING(lseek, "lseek")
SSTRING(lstat, "lstat")
SSTRING(map, "map")
SSTRING(mark, "mark")
SSTRING(max, "max")
SSTRING(mem, "mem")
SSTRING(memlock, "memlock")
SSTRING(memoize, "memoize")
SSTRING(memoized, "memoized")
SSTRING(min, "min")
SSTRING(minute, "minute")
SSTRING(mkdir, "mkdir")
SSTRING(mkfifo, "mkfifo")
SSTRING(mknod, "mknod")
SSTRING(mode, "mode")
SSTRING(module, "module")
SSTRING(month, "month")
SSTRING(mopen, "mopen")
SSTRING(msg, "msg")
SSTRING(mtime, "mtime")
SSTRING(n, "n")
SSTRING(name, "name")
SSTRING(nanos, "nanos")
SSTRING(ncollects, "ncollects")
SSTRING(net, "net")
SSTRING(new, "new")
SSTRING(nice, "nice")
SSTRING(nlink, "nlink")
SSTRING(nofile, "nofile")
SSTRING(now, "now")
SSTRING(nproc, "nproc")
SSTRING(null, "null")
SSTRING(num, "num")
SSTRING(onerror, "onerror")
SSTRING(op, "op")
SSTRING(open, "open")
SSTRING(options, "options")
SSTRING(parse, "parse")
SSTRING(parseopen, "parseopen")
SSTRING(parser, "parser")
SSTRING(parsetoken, "parsetoken")
SSTRING(parsevalue, "parsevalue")
SSTRING(passwd, "passwd")
SSTRING(path, "path")
SSTRING(pathjoin, "pathjoin")
SSTRING(pattern, "pattern")
SSTRING(pause, "pause")
SSTRING(pfopen, "pfopen")
SSTRING(pi, "pi")
SSTRING(pid, "pid")
SSTRING(pipe, "pipe")
SSTRING(pop, "pop")
SSTRING(pow, "pow")
SSTRING(print, "print")
SSTRING(printf, "printf")
SSTRING(println, "println")
SSTRING(prof, "prof")
SSTRING(profile, "profile")
SSTRING(ptr, "ptr")
SSTRING(push, "push")
SSTRING(put, "put")
SSTRING(putenv, "putenv")
SSTRING(puts, "puts")
SSTRING(rand, "rand")
SSTRING(raw, "raw")
SSTRING(rdev, "rdev")
SSTRING(rdlck, "rdlck")
SSTRING(read, "read")
SSTRING(readlink, "readlink")
SSTRING(real, "real")
SSTRING(reclaim, "reclaim")
SSTRING(recv, "recv")
SSTRING(recvfrom, "recvfrom")
SSTRING(regexp, "regexp")
SSTRING(regexpi, "regexpi")
SSTRING(rejectchar, "rejectchar")
SSTRING(rejecttoken, "rejecttoken")
SSTRING(remove, "remove")
SSTRING(rename, "rename")
SSTRING(repl, "repl")
SSTRING(replprompt, "ici>")
SSTRING(respondsto, "respondsto")
SSTRING(restore, "restore")
SSTRING(result, "result")
SSTRING(return, "return")
SSTRING(rmdir, "rmdir")
SSTRING(round, "round")
SSTRING(rpop, "rpop")
SSTRING(rpush, "rpush")
SSTRING(rss, "rss")
SSTRING(save, "save")
SSTRING(sbsize, "sbsize")
SSTRING(scope, "scope")
SSTRING(sec, "sec")
SSTRING(second, "second")
SSTRING(seek, "seek")
SSTRING(select, "select")
SSTRING(send, "send")
SSTRING(sendto, "sendto")
SSTRING(set, "set")
SSTRING(setfd, "setfd")
SSTRING(setfl, "setfl")
SSTRING(setgid, "setgid")
SSTRING(setitimer, "setitimer")
SSTRING(setlk, "setlk")
SSTRING(setown, "setown")
SSTRING(setpgrp, "setpgrp")
SSTRING(setrlimit, "setrlimit")
SSTRING(setsockopt, "setsockopt")
SSTRING(setuid, "setuid")
SSTRING(shell, "shell")
SSTRING(shutdown, "shutdown")
SSTRING(signal, "signal")
SSTRING(signam, "signam")
SSTRING(sin, "sin")
SSTRING(size, "size")
SSTRING(sktno, "sktno")
SSTRING(sktopen, "sktopen")
SSTRING(sleep, "sleep")
SSTRING(slice, "slice")
SSTRING(slosh0, "\\0")
SSTRING(sloshn, "\\n")
SSTRING(smash, "smash")
SSTRING(socket, "socket")
SSTRING(socketpair, "socketpair")
SSTRING(sopen, "sopen")
SSTRING(sort, "sort")
SSTRING(spawn, "spawn")
SSTRING(spawnp, "spawnp")
SSTRING(split, "split")
SSTRING(sprint, "sprint")
SSTRING(sprintf, "sprintf")
SSTRING(sqrt, "sqrt")
SSTRING(src, "src")
SSTRING(stack, "stack")
SSTRING(start, "start")
SSTRING(stat, "stat")
SSTRING(status, "status")
SSTRING(strbuf, "strbuf")
SSTRING(strcat, "strcat")
SSTRING(string, "string")
SSTRING(sub, "sub")
SSTRING(subject, "subject")
SSTRING(super, "super")
SSTRING(switch, "switch")
SSTRING(symlink, "symlink")
SSTRING(sync, "sync")
SSTRING(sys, "sys")
SSTRING(system, "system")
SSTRING(tan, "tan")
SSTRING(tcp, "tcp")
SSTRING(this, "this")
SSTRING(time, "time")
SSTRING(tmpname, "tmpname")
SSTRING(tochar, "tochar")
SSTRING(toint, "toint")
SSTRING(tokenobj, "tokenobj")
SSTRING(top, "top")
SSTRING(transform, "transform")
SSTRING(truncate, "truncate")
SSTRING(try, "try")
SSTRING(type, "type")
SSTRING(typecheck, "typecheck")
SSTRING(typeof, "typeof")
SSTRING(udp, "udp")
SSTRING(uid, "uid")
SSTRING(ulimit, "ulimit")
SSTRING(umask, "umask")
SSTRING(ungetchar, "ungetchar")
SSTRING(unique, "unique")
SSTRING(unknown_method, "unknown_method")
SSTRING(unlck, "unlck")
SSTRING(unlink, "unlink")
SSTRING(usec, "usec")
SSTRING(usleep, "usleep")
SSTRING(value, "value")
SSTRING(var, "var")
SSTRING(vargs, "vargs")
SSTRING(vars, "vars")
SSTRING(version, "version")
SSTRING(virtual, "virtual")
SSTRING(vstack, "vstack")
SSTRING(wait, "wait")
SSTRING(waitfor, "waitfor")
SSTRING(wakeup, "wakeup")
SSTRING(walk, "walk")
SSTRING(wday, "wday")
SSTRING(whence, "whence")
SSTRING(which, "which")
SSTRING(while, "while")
SSTRING(write, "write")
SSTRING(wrlck, "wrlck")
SSTRING(yday, "yday")
SSTRING(year, "year")
SSTRING(zone, "zone")
