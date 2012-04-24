#include <cstdio>
#include <cstring>
#include <vector>
#include <stack>

#define MEMSIZE 30000

enum Opcode {
    INC = 0, DEC, NEXT, PREV, GET, PUT, OPEN, CLOSE, END,
    CALC, MOVE, RESET_ZERO,
    MOVE_CALC, MEM_MOVE, SEARCH_ZERO,
};
const char *OPCODE_NAMES[] = {
    "+", "-", ">", "<",
    ",", ".", "[", "]", "",
    "c", "m", "z",
    "C", "M", "s",
};
union Value {
    int i1;
    struct {
        short s0, s1;
    } s2;
};
class Instruction {
public:
    Opcode op;
    Value value;
    Instruction(Opcode op) : op(op) {
    }
    Instruction(Opcode op, int i1) : op(op) {
        this->value.i1 = i1;
    }
    Instruction(Opcode op, short s0, short s1) : op(op) {
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
        if (c1.op != OPEN || c2.op != CALC || c2.value.i1 != -1 || c3.op != CLOSE)
            return;
        pop(3);
        push(Instruction(RESET_ZERO));
    }
    void check_move_calc() {
        if (insns->size() < 3)
            return;
        Instruction c1 = at(-3), c2 = at(-2), c3 = at(-1);
        if (c1.op != MOVE || c2.op != CALC || c3.op != MOVE || (-c1.value.i1 != c3.value.i1))
            return;
        short move = c1.value.i1, calc = c2.value.i1;
        pop(3);
        push(Instruction(MOVE_CALC, move, calc));
    }
    void check_mem_move() {
        if (insns->size() < 4)
            return;
        Instruction c1 = at(-4), c2 = at(-3), c3 = at(-2), c4 = at(-1);
        if (c1.op != OPEN || c4.op != CLOSE || c2.op != CALC || c2.value.i1 != -1 || c3.op != MOVE_CALC)
            return;
        pop(4);
        short move = c3.value.s2.s0;
        short calc = c3.value.s2.s1;
        push(Instruction(MEM_MOVE, move, calc));
    }
    void check_search_zero() {
        if (insns->size() < 3)
            return;
        Instruction c1 = at(-3), c2 = at(-2), c3 = at(-1);
        if (c1.op != OPEN || c2.op != MOVE || c3.op != CLOSE)
            return;
        short move = c2.value.i1;
        pop(3);
        push(Instruction(SEARCH_ZERO, move));
    }
};
class Compiler {
private:
    std::vector<Instruction>* const insns;
    int calc, move;
    std::stack<int> pcstack;
    Optimizer optimizer;
    bool is_current_op(Opcode op) {
        return insns->size() != 0 && insns->back().op == op;
    }
    void push_stack(Opcode op, int i) {
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
        push_stack(CALC, i);
    }
    void push_move(int i) {
        push_stack(MOVE, i);
        optimizer.check_move_calc();
    }
    void push_simple(Opcode op) {
        insns->push_back(Instruction(op));
    }
    void push_open() {
        pcstack.push(insns->size());
        insns->push_back(Instruction(OPEN));
    }
    void push_close() {
        int open = pcstack.top();
        int diff = insns->size() - open;
        (*insns)[open].value.i1 = diff;
        insns->push_back(Instruction(CLOSE, diff + 1));
        optimizer.check_reset_zero();
        pcstack.pop();
        optimizer.check_mem_move();
        optimizer.check_search_zero();
    }
    void push_end() {
        push_simple(END);
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
                compiler.push_simple(GET);
                break;
            case '.':
                compiler.push_simple(PUT);
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
        const char *name = OPCODE_NAMES[insn.op];
        printf("%s", name);
//        fputs(name, stdout);
        switch(insn.op) {
            case INC:
            case DEC:
            case NEXT:
            case PREV:
            case GET:
            case PUT:
            case RESET_ZERO:
                break;
            case OPEN:
            case CLOSE:
            case CALC:
            case MOVE:
            case SEARCH_ZERO:
                if (verbose) {
                    printf("(%d)", insn.value.i1);
                }
                break;
            case MOVE_CALC:
            case MEM_MOVE:
                if (verbose) {
                    printf("(%d,%d)", insn.value.s2.s0, insn.value.s2.s1);
                }
                break;
            case END:
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
            case GET:
                exec[pc].addr = &&LABEL_GET;
                break;
            case PUT:
                exec[pc].addr = &&LABEL_PUT;
                break;
            case OPEN:
                exec[pc].addr = &&LABEL_OPEN;
                break;
            case CLOSE:
                exec[pc].addr = &&LABEL_CLOSE;
                break;
            case CALC:
                exec[pc].addr = &&LABEL_CALC;
                break;
            case MOVE:
                exec[pc].addr = &&LABEL_MOVE;
                break;
            case RESET_ZERO:
                exec[pc].addr = &&LABEL_RESET_ZERO;
                break;
            case MOVE_CALC:
                exec[pc].addr = &&LABEL_MOVE_CALC;
                break;
            case MEM_MOVE:
                exec[pc].addr = &&LABEL_MEM_MOVE;
                break;
            case SEARCH_ZERO:
                exec[pc].addr = &&LABEL_SEARCH_ZERO;
                break;
            case END:
                exec[pc].addr = &&LABEL_END;
                goto LABEL_START;
            default:
                return;
        }
    }
LABEL_START:
    int *mem = membuf;
    ExeCode *pc = exec - 1;

#define NEXT_LABEL \
    ++pc; \
    goto *pc->addr

    NEXT_LABEL;
LABEL_GET:
    *mem = getchar();
    NEXT_LABEL;
LABEL_PUT:
    putchar(*mem);
    NEXT_LABEL;
LABEL_OPEN:
    if (*mem == 0) {
        pc += pc->value.i1;
    }
    NEXT_LABEL;
LABEL_CLOSE:
    pc -= pc->value.i1;
    NEXT_LABEL;
LABEL_CALC:
    *mem += pc->value.i1;
    NEXT_LABEL;
LABEL_MOVE:
    mem += pc->value.i1;
    NEXT_LABEL;
LABEL_RESET_ZERO:
    *mem = 0;
    NEXT_LABEL;
LABEL_MOVE_CALC:
    mem[pc->value.s2.s0] += pc->value.s2.s1;
    NEXT_LABEL;
LABEL_MEM_MOVE:
    mem[pc->value.s2.s0] += *mem * pc->value.s2.s1;
    *mem = 0;
    NEXT_LABEL;
LABEL_SEARCH_ZERO:
    int search_zero = pc->value.i1;
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
