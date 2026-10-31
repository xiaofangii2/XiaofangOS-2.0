#include "shell.h"
#include "sys.h"
#include "sha256.h"
#define MAX_CMD_LEN 256
#define MAX_ARGS 32
typedef struct { const char *name; void (*fn)(int argc, char **argv); } cmd_t;
static char g_user[64]="root";
static char g_cwd[256]="/home/root";
static int g_running=1;
static char g_prompt[384];
static void cmd_help(int,char**); static void cmd_exit(int,char**); static void cmd_sy(int,char**); static void cmd_me(int,char**);
static void cmd_list(int,char**); static void cmd_new(int,char**); static void cmd_open(int,char**); static void cmd_del(int,char**);
static void cmd_nu(int,char**); static void cmd_who(int,char**); static void cmd_switch(int,char**); static void cmd_pass(int,char**);
static void cmd_cd(int,char**); static void cmd_here(int,char**);
static cmd_t g_cmds[]={{"help",cmd_help},{"exit",cmd_exit},{"sy",cmd_sy},{"me",cmd_me},{"list",cmd_list},{"new",cmd_new},{"open",cmd_open},{"del",cmd_del},{"nu",cmd_nu},{"who",cmd_who},{"switch",cmd_switch},{"pass",cmd_pass},{"cd",cmd_cd},{"here",cmd_here}};
#define CMD_COUNT (sizeof(g_cmds)/sizeof(g_cmds[0]))
static void read_line(char *buf,int max){ int len=0; buf[0]=0; while(1){ char c; long n=sys_read(0,&c,1); if(n<=0) return; if(c=='\n'||c=='\r'){ return; } if(c==127||c==8){ if(len>0){ len--; puts_("\b \b"); } continue; } if(c<32) continue; if(len>=max-1) continue; buf[len++]=c; buf[len]=0; putc_(c); } }
static void dispatch(int argc,char **argv){ if(argc==0) return; for(unsigned long i=0;i<CMD_COUNT;i++){ if(scmp(argv[0],g_cmds[i].name)==0){ g_cmds[i].fn(argc,argv); return; } } puts_("未知命令: "); puts_line(argv[0]); }
static int read_file_line(const char *path,char *out,int max){ long fd=sys_openat(-100,path,0,0); if(fd<0) return 0; int n=0; char c; while(n<max-1&&sys_read(fd,&c,1)==1){ if(c=='\n'||c=='\r') break; out[n++]=c; } out[n]=0; sys_close(fd); return n>0; }
static int write_file(const char *path,const char *s){ long fd=sys_openat(-100,path,0x241,0644); if(fd<0) return 0; sys_write(fd,s,slen(s)); sys_close(fd); return 1; }
static int file_exists(const char *path){ long fd=sys_openat(-100,path,0,0); if(fd>=0){ sys_close(fd); return 1; } return 0; }
static void mkdir_p(const char *path){ sys_mkdirat(-100,path,0755); }
static int user_exists(const char *name){ long fd=sys_openat(-100,"/etc/user",0,0); if(fd<0) return 0; char buf[128]; int n=0; char c; int found=0; while(sys_read(fd,&c,1)==1){ if(c=='\n'){ buf[n]=0; if(scmp(buf,name)==0){found=1;break;} n=0; } else buf[n++]=c; } if(!found&&n>0){ buf[n]=0; if(scmp(buf,name)==0) found=1; } sys_close(fd); return found; }
static void add_user(const char *name){ long fd=sys_openat(-100,"/etc/user",0x441,0644); if(fd<0) return; sys_write(fd,name,slen(name)); sys_write(fd,"\n",1); sys_close(fd); }
static int load_password(const char *name,char out[65]){ char p[256]; scpy(p,"/var/xiaofang/",sizeof(p)); scpy(p+slen(p),name,sizeof(p)-slen(p)); scpy(p+slen(p),".password",sizeof(p)-slen(p)); return read_file_line(p,out,65); }
static void save_password(const char *name,const char *pass){ char p[256]; scpy(p,"/var/xiaofang/",sizeof(p)); scpy(p+slen(p),name,sizeof(p)-slen(p)); scpy(p+slen(p),".password",sizeof(p)-slen(p)); char hex[65]; sha256_hex(pass,hex); write_file(p,hex); }
static int check_password(const char *name,const char *pass){ char stored[65]; if(!load_password(name,stored)) return 1; char hex[65]; sha256_hex(pass,hex); return scmp(stored,hex)==0; }
static void read_password(const char *label,char *out,int max){ puts_(label); int i=0; while(i<max-1){ char c; if(sys_read(0,&c,1)<=0) break; if(c=='\n'||c=='\r') break; out[i++]=c; } out[i]=0; putc_('\n'); }
static void oobe(void){
    if(file_exists("/etc/oobe")) return;
    puts_line(""); puts_line("=== 欢迎使用 XiaofangOS ==="); puts_line("首次启动，先做几项设置。"); puts_line("");
    char region[64], keyboard[64], user[64], pass[128];
    puts_("地区: "); { int i=0; while(i<63){ char c; if(sys_read(0,&c,1)<=0)break; if(c=='\n'||c=='\r')break; region[i++]=c;} region[i]=0; putc_('\n'); }
    puts_("键盘布局: "); { int i=0; while(i<63){ char c; if(sys_read(0,&c,1)<=0)break; if(c=='\n'||c=='\r')break; keyboard[i++]=c;} keyboard[i]=0; putc_('\n'); }
    puts_("用户名: "); { int i=0; while(i<63){ char c; if(sys_read(0,&c,1)<=0)break; if(c=='\n'||c=='\r')break; user[i++]=c;} user[i]=0; putc_('\n'); }
    if(user[0]==0){ scpy(user,"root",sizeof(user)); }
    read_password("密码（留空不设）: ", pass, sizeof(pass));
    scpy(g_user, user, sizeof(g_user));
    write_file("/etc/user", "root\n");
    if(scmp(user,"root")!=0){ long fd=sys_openat(-100,"/etc/user",0x441,0644); if(fd>=0){ sys_write(fd,user,slen(user)); sys_write(fd,"\n",1); sys_close(fd);} }
    write_file("/etc/users", user);
    if(pass[0]) save_password(user, pass);
    char d[256]; scpy(d,"/home/",sizeof(d)); scpy(d+slen(d),user,sizeof(d)-slen(d)); mkdir_p(d);
    scpy(g_cwd,"/home/",sizeof(g_cwd)); scpy(g_cwd+slen(g_cwd),user,sizeof(g_cwd)-slen(g_cwd));
    write_file("/etc/oobe","true\n");
    puts_line("设置完成。"); puts_line("");
}
static void login(void){ char stored[65]; if(!load_password(g_user,stored)) return; int tries=0; while(tries<3){ char pass[128]; char p[128]; scpy(p,g_user,sizeof(p)); scpy(p+slen(p)," 的密码: ",sizeof(p)-slen(p)); read_password(p,pass,sizeof(pass)); if(check_password(g_user,pass)) return; puts_line("密码错误"); tries++; } puts_line("密码错误次数过多，退出。"); g_running=0; }
void shell_prompt(const char *user){ (void)user; char tmp[64]; mkdir_p("/home"); mkdir_p("/etc"); mkdir_p("/var"); mkdir_p("/var/xiaofang"); oobe(); if(read_file_line("/etc/users",tmp,sizeof(tmp))&&tmp[0]){ scpy(g_user,tmp,sizeof(g_user)); scpy(g_cwd,"/home/",sizeof(g_cwd)); scpy(g_cwd+slen(g_cwd),tmp,sizeof(g_cwd)-slen(g_cwd)); } login(); if(!g_running) return; static char line[MAX_CMD_LEN]; for(;;){ g_prompt[0]=0; scpy(g_prompt,g_user,sizeof(g_prompt)); unsigned long l=slen(g_prompt); g_prompt[l++]=':'; scpy(g_prompt+l,g_cwd,sizeof(g_prompt)-l); l=slen(g_prompt); g_prompt[l++]='/'; g_prompt[l++]=' '; g_prompt[l]=0; puts_(g_prompt); read_line(line,MAX_CMD_LEN); if(line[0]==0) continue; char *argv[MAX_ARGS]; int argc=ssplit(line,argv,MAX_ARGS); dispatch(argc,argv); if(!g_running) break; } }
static void cmd_help(int argc,char **argv){ (void)argc;(void)argv; puts_line("命令:"); for(unsigned long i=0;i<CMD_COUNT;i++){ puts_("  "); puts_line(g_cmds[i].name);} }
static void cmd_exit(int argc,char **argv){ (void)argc;(void)argv; g_running=0; }
static void cmd_sy(int argc,char **argv){ (void)argc;(void)argv; puts_line("XiaofangOS 2.0"); }
static void cmd_me(int argc,char **argv){ (void)argc;(void)argv; puts_line(g_user); }
static void cmd_list(int argc,char **argv){
    (void)argc;(void)argv;
    long fd = sys_openat(-100, g_cwd, 0x10000, 0);
    if (fd < 0) { puts_line("(无法打开目录)"); return; }
    char buf[2048];
    long n;
    int count = 0;
    while ((n = sys_getdents64(fd, buf, sizeof(buf))) > 0) {
        long off = 0;
        while (off < n) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + off);
            if (scmp(d->d_name, ".") != 0 && scmp(d->d_name, "..") != 0) {
                puts_("  ");
                puts_line(d->d_name);
                count++;
            }
            off += d->d_reclen;
        }
    }
    sys_close(fd);
    if (count == 0) puts_line("  (空)");
}
static void cmd_new(int argc,char **argv){ if(argc<2){puts_line("用法: new <file>");return;} char p[512]; scpy(p,g_cwd,sizeof(p)); scpy(p+slen(p),"/",sizeof(p)-slen(p)); scpy(p+slen(p),argv[1],sizeof(p)-slen(p)); write_file(p,""); puts_("已创建: "); puts_line(argv[1]); }
static void cmd_open(int argc,char **argv){ if(argc<2){puts_line("用法: open <file>");return;} char p[512]; scpy(p,g_cwd,sizeof(p)); scpy(p+slen(p),"/",sizeof(p)-slen(p)); scpy(p+slen(p),argv[1],sizeof(p)-slen(p)); long fd=sys_openat(-100,p,0,0); if(fd<0){puts_line("不存在");return;} char buf[512]; long n; while((n=sys_read(fd,buf,sizeof(buf)))>0) sys_write(1,buf,n); sys_close(fd); putc_('\n'); }
static void cmd_del(int argc,char **argv){ if(argc<2){puts_line("用法: del <file>");return;} char p[512]; scpy(p,g_cwd,sizeof(p)); scpy(p+slen(p),"/",sizeof(p)-slen(p)); scpy(p+slen(p),argv[1],sizeof(p)-slen(p)); long r=sys_unlinkat(-100,p,0); if(r<0) puts_line("删除失败"); else { puts_("已删除: "); puts_line(argv[1]); } }
static void cmd_nu(int argc,char **argv){ if(argc<2){puts_line("用法: nu <用户>");return;} if(user_exists(argv[1])){puts_line("用户已存在");return;} add_user(argv[1]); char d[256]; scpy(d,"/home/",sizeof(d)); scpy(d+slen(d),argv[1],sizeof(d)-slen(d)); mkdir_p(d); puts_("已创建: "); puts_line(argv[1]); char pass[128]; read_password("密码（留空不设）: ",pass,sizeof(pass)); if(pass[0]) save_password(argv[1],pass); }
static void cmd_who(int argc,char **argv){ (void)argc;(void)argv; puts_line("用户列表:"); long fd=sys_openat(-100,"/etc/user",0,0); if(fd<0){puts_line("  root");return;} char buf[128]; int n=0; char c; while(sys_read(fd,&c,1)==1){ if(c=='\n'){buf[n]=0;if(n>0){puts_("  ");puts_line(buf);}n=0;} else buf[n++]=c; } if(n>0){buf[n]=0;puts_("  ");puts_line(buf);} sys_close(fd); }
static void cmd_switch(int argc,char **argv){ if(argc<2){puts_line("用法: switch <用户>");return;} if(!user_exists(argv[1])){puts_line("未知用户");return;} char stored[65]; if(load_password(argv[1],stored)){ char pass[128]; read_password("密码: ",pass,sizeof(pass)); if(!check_password(argv[1],pass)){puts_line("密码错误");return;} } scpy(g_user,argv[1],sizeof(g_user)); write_file("/etc/users",argv[1]); scpy(g_cwd,"/home/",sizeof(g_cwd)); scpy(g_cwd+slen(g_cwd),argv[1],sizeof(g_cwd)-slen(g_cwd)); char d[256]; scpy(d,"/home/",sizeof(d)); scpy(d+slen(d),argv[1],sizeof(d)-slen(d)); mkdir_p(d); puts_("已切换: "); puts_line(argv[1]); }
static void cmd_pass(int argc,char **argv){ (void)argc;(void)argv; char old[128],fresh[128]; read_password("当前密码: ",old,sizeof(old)); if(!check_password(g_user,old)){puts_line("密码错误");return;} read_password("新密码: ",fresh,sizeof(fresh)); if(!fresh[0]){puts_line("取消");return;} save_password(g_user,fresh); puts_line("密码已更新"); }
static void cmd_cd(int argc,char **argv){ if(argc<2){ scpy(g_cwd,"/home/",sizeof(g_cwd)); scpy(g_cwd+slen(g_cwd),g_user,sizeof(g_cwd)-slen(g_cwd)); return; } if(scmp(argv[1],"/")==0){ scpy(g_cwd,"/",sizeof(g_cwd)); return; } if(argv[1][0]=='/'){ scpy(g_cwd,argv[1],sizeof(g_cwd)); return; } scpy(g_cwd,"/home/",sizeof(g_cwd)); scpy(g_cwd+slen(g_cwd),g_user,sizeof(g_cwd)-slen(g_cwd)); scpy(g_cwd+slen(g_cwd),"/",sizeof(g_cwd)-slen(g_cwd)); scpy(g_cwd+slen(g_cwd),argv[1],sizeof(g_cwd)-slen(g_cwd)); }
static void cmd_here(int argc,char **argv){ (void)argc;(void)argv; puts_line(g_cwd); }
