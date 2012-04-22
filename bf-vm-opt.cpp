#include <cstdio>
#include <cstring>
#include <vector>
#include <stack>

#define MEMSIZE 30000

union Value {
    int i1;
    struct {
        short s0, s1;
    } s2;
};
class Instruction {
public:
    char op;
    Value value;
    Instruction(char op) : op(op) {
    }
    Instruction(char op, int i1) : op(op) {
        this->value.i1 = i1;
    }
    Instruction(char op, short s0, short s1) : op(op) {
        this->value.s2.s0 = s0;
        this->value.s2.s1 = s1;
    }

};
struct ExeCode {
    void *addr;
    Value value;
};
class Optimizer {
private:
    std::vector<Instruction>* const insns;
public:
    Optimizer(std::vector<Instruction>* const insns) :
        insns(insns) {
    }
    void push(Instruction insn) {
        insns->push_back(insn);
    }
    void pop(const int count) {
        for (int i = 0; i < count; ++i)
            insns->pop_back();
    }
    Instruction& at(const int i) {
        return insns->at(i < 0 ? insns->size() + i : i);
    }
    void check_reset_zero() {
        if (insns->size() < 3)
            return;
        Instruction c1 = at(-3), c2 = at(-2), c3 = at(-1);
        if (c1.op != '[' || c2.op != 'c' || c2.value.i1 != -1 || c3.op != ']')
            return;
        pop(3);
        push(Instruction('z'));
    }
    void check_move_calc() {
        if (insns->size() < 3)
            return;
        Instruction c1 = at(-3), c2 = at(-2), c3 = at(-1);
        if (c1.op != 'm' || c2.op != 'c' || c3.op != 'm' || (-c1.value.i1 != c3.value.i1))
            return;
        short move = c1.value.i1, calc = c2.value.i1;
        pop(3);
        push(Instruction('C', move, calc));
    }
    void check_mem_move() {
        if (insns->size() < 4)
            return;
        Instruction c1 = at(-4), c2 = at(-3), c3 = at(-2), c4 = at(-1);
        if (c1.op != '[' || c4.op != ']' || c2.op != 'c' || c2.value.i1 != -1 || c3.op != 'C')
            return;
        pop(4);
        short move = c3.value.s2.s0;
        short calc = c3.value.s2.s1;
        push(Instruction('M', move, calc));
    }
    void check_search_zero() {
        if (insns->size() < 3)
            return;
        Instruction c1 = at(-3), c2 = at(-2), c3 = at(-1);
        if (c1.op != '[' || c2.op != 'm' || c3.op != ']')
            return;
        short move = c2.value.i1;
        pop(3);
        push(Instruction('s', move));
    }
};
class Compiler {
private:
    std::vector<Instruction>* const insns;
    int calc, move;
    std::stack<int> pcstack;
    Optimizer optimizer;
    bool is_current_op(char op) {
        return insns->size() != 0 && insns->back().op == op;
    }
    void push_stack(char op, int i) {
        if (is_current_op(op)) {
            insns->back().value.i1 += i;
        } else {
            insns->push_back(Instruction(op, i));
        }
    }
public:
    Compiler(std::vector<Instruction>* insns) :
        insns(insns), calc(0), move(0), optimizer(insns) {
    }
    void push_calc(int i) {
        push_stack('c', i);
    }
    void push_move(int i) {
        push_stack('m', i);
        optimizer.check_move_calc();
    }
    void push_simple(char op) {
        insns->push_back(Instruction(op));
    }
    void push_open() {
        pcstack.push(insns->size());
        insns->push_back(Instruction('['));
    }
    void push_close() {
        int open = pcstack.top();
        (*insns)[open].value.i1 = insns->size();
        insns->push_back(Instruction(']', open - 1));
        optimizer.check_reset_zero();
        pcstack.pop();
        optimizer.check_mem_move();
        optimizer.check_search_zero();
    }
    void push_end() {
        push_simple('\0');
    }
};
void parse(std::vector<Instruction> &insns, FILE *input) {
    Compiler compiler(&insns);
    int ch = 0;
    while ((ch=getc(input)) != EOF) {
        switch (ch) {
            case '+':
                compiler.push_calc(1);
                break;
            case '-':
                compiler.push_calc(-1);
                break;
            case '>':
                compiler.push_move(1);
                break;
            case '<':
                compiler.push_move(-1);
                break;
            case ',':
            case '.':
                compiler.push_simple(ch);
                break;
            case '[':
                compiler.push_open();
                break;
            case ']':
                compiler.push_close();
                break;
        }
    }
    compiler.push_end();
}
void debug(std::vector<Instruction> &insns, bool verbose) {
    for (size_t pc=0;;++pc) {
        Instruction insn = insns[pc];
        switch(insn.op) {
            case '+':
            case '-':
            case '>':
            case '<':
            case ',':
            case '.':
            case 'z':
                putchar(insn.op);
                break;
            case '[':
            case ']':
            case 'c':
            case 'm':
            case 's':
                putchar(insn.op);
                if (verbose) {
                    printf("(%d)", insn.value.i1);
                }
                break;
            case 'C':
            case 'M':
                putchar(insn.op);
                if (verbose) {
                    printf("(%d,%d)", insn.value.s2.s0, insn.value.s2.s1);
                }
                break;
            case '\0':
                return;
        }
    }
}
void execute(std::vector<Instruction> &insns, int membuf[MEMSIZE]) {
    ExeCode exec[insns.size()];
    for (size_t pc=0;;++pc) {
        Instruction insn = insns[pc];
        exec[pc].value = insn.value;
        switch(insn.op) {
            case ',':
                exec[pc].addr = &&LABEL_GET;
                break;
            case '.':
                exec[pc].addr = &&LABEL_PUT;
                break;
            case '[':
                exec[pc].addr = &&LABEL_OPEN;
                break;
            case ']':
                exec[pc].addr = &&LABEL_CLOSE;
                break;
            case 'c':
                exec[pc].addr = &&LABEL_CALC;
                break;
            case 'm':
                exec[pc].addr = &&LABEL_MOVE;
                break;
            case 'z':
                exec[pc].addr = &&LABEL_RESET;
                break;
            case 'C':
                exec[pc].addr = &&LABEL_MOVE_CALC;
                break;
            case 'M':
                exec[pc].addr = &&LABEL_MEM_MOVE;
                break;
            case 's':
                exec[pc].addr = &&LABEL_SEARCH_ZERO;
                break;
            case '\0':
                exec[pc].addr = &&LABEL_END;
                goto LABEL_START;
        }
    }
LABEL_START:
    int *mem = membuf;
    int pc = -1;
    ExeCode ecode;

#define NEXT_LABEL \
    ecode = exec[++pc]; \
    goto *ecode.addr

    NEXT_LABEL;
LABEL_GET:
    *mem = getchar();
    NEXT_LABEL;
LABEL_PUT:
    putchar(*mem);
    NEXT_LABEL;
LABEL_OPEN:
    if (*mem == 0) {
        pc = ecode.value.i1;
    }
    NEXT_LABEL;
LABEL_CLOSE:
    pc = ecode.value.i1;
    NEXT_LABEL;
LABEL_CALC:
    *mem += ecode.value.i1;
    NEXT_LABEL;
LABEL_MOVE:
    mem += ecode.value.i1;
    NEXT_LABEL;
LABEL_RESET:
    *mem = 0;
    NEXT_LABEL;
LABEL_MOVE_CALC:
    mem[ecode.value.s2.s0] += ecode.value.s2.s1;
    NEXT_LABEL;
LABEL_MEM_MOVE:
    mem[ecode.value.s2.s0] += *mem * ecode.value.s2.s1;
    *mem = 0;
    NEXT_LABEL;
LABEL_SEARCH_ZERO:
    int search_zero = ecode.value.i1;
    while (*mem != 0) {
        mem += search_zero;
    }
    NEXT_LABEL;
LABEL_END:
    ;
}
int main(int argc, char *argv[]) {
    static int membuf[MEMSIZE];
    std::vector<Instruction> insns;
    parse(insns, stdin);
    if (argc == 1) {
        execute(insns, membuf);
    } else if (argc == 2) {
        const char *option = argv[1];
        if (strcmp(option, "-debug") == 0) {
            debug(insns, false);
        } else if (strcmp(option, "-debug-verbose") == 0) {
            debug(insns, true);
        }
    }
    return 0;
}
