#include <stdint.h>
#define UB 0x09000000UL
#define DR (*(volatile uint32_t *)(UB + 0x00))
#define FR (*(volatile uint32_t *)(UB + 0x18))
#define CR (*(volatile uint32_t *)(UB + 0x30))
static void pc(char c) { while (FR & (1 << 5)) { } DR = (uint32_t)c; }
static void ps(const char *s) { while (*s) { if (*s == '\n') pc('\r'); pc(*s++); } }
static int eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static char U[32] = "root";
static char L[64];
static char P[32];
static char FN[8][32];
static char FC[8][64];
static int NF = 0;
static char UN[8][32];
static int NU = 1;
static int RN, I, J, F;
static char RC, T[32];
static void rd(char *b, int m) {
    RN = 0; b[0] = 0;
    while (1) {
        while (FR & (1 << 4)) { }
        RC = (char)(DR & 0xFF);
        if (RC == '\r' || RC == '\n') { ps("\n"); b[RN] = 0; return; }
        if (RC == 127 || RC == 8) { if (RN > 0) { RN--; ps("\b \b"); } continue; }
        if (RC < 32) continue;
        if (RN >= m - 1) continue;
        b[RN++] = RC; pc(RC);
    }
}
void kmain(void) {
    CR = (1 << 0) | (1 << 8) | (1 << 9);
    ps("\n正在启动 XiaofangOS 2.0...\n启动完成\n\n");
    ps("=== 欢迎使用 XiaofangOS ===\n首次启动，先做几项设置。\n\n");
    ps("地区: "); rd(L, 64);
    ps("键盘布局: "); rd(L, 64);
    ps("用户名: "); rd(U, 32);
    if (U[0] == 0) { U[0]='r'; U[1]='o'; U[2]='o'; U[3]='t'; U[4]=0; }
    ps("密码: "); rd(P, 32);
    ps("设置完成。\n\n");
    UN[0][0]='r'; UN[0][1]='o'; UN[0][2]='o'; UN[0][3]='t'; UN[0][4]=0; NU = 1;
    if (!eq(U, "root")) { I = 0; while (U[I]) { UN[1][I] = U[I]; I++; } UN[1][I] = 0; NU = 2; }
    while (1) {
        ps(U); ps(":/ ");
        rd(L, 64);
        if (L[0] == 0) continue;
        if (eq(L, "exit")) { ps("再见。\n"); for (;;) { } }
        else if (eq(L, "help")) {
            ps("命令:\n");
            ps("  help\n");
            ps("  exit\n");
            ps("  sy\n");
            ps("  me\n");
            ps("  list\n");
            ps("  new\n");
            ps("  open\n");
            ps("  del\n");
            ps("  nu\n");
            ps("  who\n");
            ps("  switch\n");
            ps("  pass\n");
            ps("  cd\n");
            ps("  here\n");
        }
        else if (eq(L, "sy")) { ps("XiaofangOS 2.0 (bare)\n"); }
        else if (eq(L, "me")) { ps(U); ps("\n"); }
        else if (eq(L, "list")) {
            if (NF == 0) ps("(空)\n");
            else { for (I = 0; I < NF; I++) { ps("  "); ps(FN[I]); ps("\n"); } }
        }
        else if (L[0]=='n' && L[1]=='e' && L[2]=='w' && L[3]==' ') {
            if (NF >= 8) { ps("文件满了\n"); continue; }
            I = 4; J = 0;
            while (L[I] && L[I] != ' ' && J < 31) { FN[NF][J++] = L[I++]; }
            FN[NF][J] = 0; FC[NF][0] = 0;
            ps("已创建: "); ps(FN[NF]); ps("\n");
            NF++;
        }
        else if (L[0]=='o' && L[1]=='p' && L[2]=='e' && L[3]=='n' && L[4]==' ') {
            I = 5; J = 0;
            while (L[I] && L[I] != ' ' && J < 31) { T[J++] = L[I++]; }
            T[J] = 0;
            F = -1;
            for (I = 0; I < NF; I++) if (eq(FN[I], T)) { F = I; break; }
            if (F < 0) ps("不存在\n");
            else { ps(FC[F]); ps("\n"); }
        }
        else if (L[0]=='d' && L[1]=='e' && L[2]=='l' && L[3]==' ') {
            I = 4; J = 0;
            while (L[I] && L[I] != ' ' && J < 31) { T[J++] = L[I++]; }
            T[J] = 0;
            F = -1;
            for (I = 0; I < NF; I++) if (eq(FN[I], T)) { F = I; break; }
            if (F < 0) ps("不存在\n");
            else {
                for (I = F; I < NF - 1; I++) { J = 0; while (FN[I+1][J]) { FN[I][J]=FN[I+1][J]; J++; } FN[I][J]=0; }
                NF--; ps("已删除\n");
            }
        }
        else if (L[0]=='n' && L[1]=='u' && L[2]==' ') {
            if (NU >= 8) { ps("用户满了\n"); continue; }
            I = 3; J = 0;
            while (L[I] && L[I] != ' ' && J < 31) { UN[NU][J++] = L[I++]; }
            UN[NU][J] = 0;
            ps("已创建: "); ps(UN[NU]); ps("\n");
            NU++;
        }
        else if (eq(L, "who")) {
            ps("用户:\n");
            for (I = 0; I < NU; I++) { ps("  "); ps(UN[I]); ps("\n"); }
        }
        else if (L[0]=='s' && L[1]=='w' && L[2]=='i' && L[3]=='t' && L[4]=='c' && L[5]=='h' && L[6]==' ') {
            I = 7; J = 0;
            while (L[I] && L[I] != ' ' && J < 31) { T[J++] = L[I++]; }
            T[J] = 0;
            F = -1;
            for (I = 0; I < NU; I++) if (eq(UN[I], T)) { F = I; break; }
            if (F < 0) ps("未知用户\n");
            else { J=0; while(T[J]){ U[J]=T[J]; J++; } U[J]=0; ps("已切换: "); ps(U); ps("\n"); }
        }
        else if (eq(L, "pass")) {
            ps("新密码: "); rd(P, 32); ps("密码已更新\n");
        }
        else if (eq(L, "cd")) { ps("在 /home/"); ps(U); ps("\n"); }
        else if (eq(L, "here")) { ps("/home/"); ps(U); ps("\n"); }
        else { ps("未知命令: "); ps(L); ps("\n"); }
    }
}
