首先，把符号表与汇编代码导出来：
- objdump -t bomb > bomb.symbols
- objdump -d bomb > bomb.s

### 第一关

- 看汇编可以看到启动炸弹爆炸是通过函数explode_bomb启动的，因此在此打断点。
- 然后在phase_1打断点。
- 通过观察汇编代码，发现mov $0x402400,%esi这条指令后即调用strings_not_equal
- 打印%esi这个地址的字符串：x %esi
- 通关！

### 第二关

- 看汇编代码，可以看到read_six_numbers，盲猜输入六个数字，随便输入进去看看。
- 通过cmpl $0x01 (%rsp)，说明第一个数字一定为1。
- 仔细分析汇编代码，可以看出此代码要求输入是比例为2的等比数列。
- 输入：1 2 4 8 16 32，通关！

### 第三关

- 首先汇编代码第四行的$0x4025cf有点诡异，打印一下：x/sb 0x4025cf会显示"%d %d"，得到输入格式。
- 紧接着看到第一个数要比7小，第一个数存在%rax中。
- jmpq   \*0x402470(,%rax,8) 指令意思是从 0x402470+8\*%rax地址中取出新的地址值作为跳转地址，一般由switch语句翻译而来。
- 接着跳转到相应分支查看第二个数为多少，如果第一个数为1，则跳转分支要求的第二个数值为311。
- 通关！

### 第四关

- 前面同第三关一样，可以看到输入格式也是"%d %d"
- 第一个数字要小于等于14
- 然后进入func4看汇编代码，可以看出是一个递归函数，初始输入是func4(a, 0, 14)，其中a是我们输入的第一个数值。
```cpp
int func4(int a, int b, int c) {
    int eax = (c - b) >> 1;
    int ecx = eax + b;
    if (ecx > a) {
        return 2 * func4(a, b, ecx - 1);
    } else if (ecx == a){
        return 0;
    } else {
        return 2 * func4(a, ecx + 1, c) + 1;
    }
}
```
- 不触发爆炸的要求是func4返回0，因此令a = 7即可（除此之外，3、1、0都可以）。
- 回到phase_4，第二个数值要求是0
- 通关！

### 第五关

- 观察汇编代码，我写了注释翻译了一番，输入的六个字符取ASCII码的低四位作为坐标index，去一串固定字符串中取6个字符。
- 要求取出的字符为"flyers"
- 通关！

### 第六关

- 汇编代码非常长，逐句翻译，写下注释
- 首先要输入六个数字，这六个数字要互不相同（其实应该是编号1 - 6）
- 每个数字分别用7减去作为之后的index序列
- 在0x6032d0这个地方放着一串结构体链表：
```cpp
struct node {
    int val;
    int index;
    node* next;
};
```
- 之后会按照前面得到的index序列重新链起这串链表
- 要求最后链表的val值是从大到小排列
- 通关！

### 隐藏关卡

- 在phase_6下面即发现有一个secret_phase，通过搜索可以看到在phase_defused中会调用该函数
- 观察phase_defused发现，中间的逻辑会在输入第六个input后触发
- 进一步观察，sscanf的输入格式是"%d %d %s"，输入地址是第四关的字符串，要求后面的字符串应该为"DrEvil"
- 之后进入secret_phase，发现要求多输入一行数字，这个数字要小于等于1001，之后调用fun7，要求返回值等于2。
```cpp
struct node {
    long long val;
    node* left;
    node* right;
};
int fun7(node* a, long b) {
    if (a == nullptr) return -1;
    int tmp = a->val;
    if (tmp <= b) {
        if (tmp == b) {
            return 0;
        } else {
            return 2 * fun7(a->right, b) + 1;
        }
    } else {
        return 2 * fun7(a->left, b);
    }
}
Tree: 
                            0x24
            0x8                             0x32
    0x6             0x16            0x2d            0x6b
0x1     0x7     0x14    0x23    0x28    0x2f    0x63    0x3e9
```
- 返回值是2，因此必须是2 * (2 * 0 + 1)的形式，按调用堆栈逆序就是先左后右，所以应该输入0x16，十进制为22。
- 通关！