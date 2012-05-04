#include <cstdio>
#include <cstring>
#include <vector>
#include <stack>
#include <iostream>

#include <xbyak/xbyak.h>

#define MEMSIZE 30000
#define CODESIZE 50000

enum Opcode {
    INC = 0, DEC, NEXT, PREV, GET, PUT, OPEN, CLOSE, END,
    CALC, MOVE, RESET_ZERO,
    MOVE_CALC, MEM_MOVE, SEARCH_ZERO, LOAD,
    OPEN_FAST, CLOSE_FAST, CALC_FAST
};
const char *OPCODE_NAMES[] = {
    "+", "-", ">", "<",
    ",", ".", "[", "]", "",
    "c", "m", "z",
    "C", "M", "s", "l",
    "{", "}", "F",
    "N"
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
    Instruction(Opcode op, Value value) : op(op), value(value) {
    }
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
    int calc_value(Instruction insn) {
        switch (insn.op) {
        case CALC:
            return insn.value.i1;
        case INC:
            return 1;
        case DEC:
            return -1;
        default:
            return 0;
        }
    }
    int move_value(Instruction insn) {
        switch (insn.op) {
        case MOVE:
            return insn.value.i1;
        case NEXT:
            return 1;
        case PREV:
            return -1;
        default:
            return 0;
        }
    }
    int move_value_for_index_calculation(Instruction insn) {
        switch (insn.op) {
        case MEM_MOVE:
            return insn.value.s2.s0;
        case MOVE:
            return insn.value.i1;
        case NEXT:
            return 1;
        case PREV:
            return -1;
        default:
            return 0;
        }
    }
    bool is_undeterminable_move(Instruction insn) {
        switch(insn.op) {
        case SEARCH_ZERO:
            return true;
        default:
            return false;
        }
    }
    bool is_ecx_used(Instruction insn) {
        switch (insn.op) {
            case OPEN_FAST:
            case CLOSE_FAST:
            case CALC_FAST:
            case GET:
            case PUT:
                return true;
            default:
                return false;
        }
    }
    void check_calc() {
        // c(n)c(m) -> c(n+m)
        if (insns->size() < 2)
            return;
        Instruction c1 = at(-2), c2 = at(-1);
        int val1 = calc_value(c1), val2 = calc_value(c2);
        if (val1 == 0 || val2 == 0)
            return;
        pop(2);
        if (val1 + val2 != 0)
            push(Instruction(CALC, val1 + val2));
    }
    void check_move() {
        if (insns->size() < 2)
            return;
        Instruction c1 = at(-2), c2 = at(-1);
        int val1 = move_value(c1), val2 = move_value(c2);
        if (val1 == 0 || val2 == 0)
            return;
        pop(2);
        if (val1 + val2 != 0)
            push(Instruction(MOVE, val1 + val2));
    }
    void check_reset_zero() {
        // [c(-1)] -> l(0)
        if (insns->size() < 3)
            return;
        Instruction c1 = at(-3), c2 = at(-2), c3 = at(-1);
        int val2 = calc_value(c2);
        if (c1.op != OPEN || c2.op != CALC || val2 != -1 || c3.op != CLOSE)
            return;
        pop(3);
        push(Instruction(LOAD,0));
    }
    void check_load() {
        // l(x)c(y) -> l(x+y)
        if (insns->size() < 2)
            return;
        Instruction c1 = at(-2), c2 = at(-1);
        int val2=calc_value(c2);
        if (c1.op != LOAD || val2 == 0)
            return;
        pop(2);
        push(Instruction(LOAD, c1.value.i1 + val2));
    }
    void check_load_dup() {
        // l(x)l(y) -> l(y)
        if (insns->size() < 2)
            return;
        Instruction c1 = at(-2), c2 = at(-1);
        if (c1.op != LOAD || c2.op != LOAD)
            return;
        pop(2);
        push(c2);
    }
    void check_move_calc() {
        // m(n),c(x),m(-n) -> C(n,x)
        if (insns->size() < 3)
            return;
        Instruction c1 = at(-3), c2 = at(-2), c3 = at(-1);
        int val1 = move_value(c1), val2 = calc_value(c2), val3 = move_value(c3);
        if (val1 == 0 || val2 == 0 || val3 == 0 || (-val1 != val3))
            return;
        short move = val1, calc = val2;
        pop(3);
        push(Instruction(MOVE_CALC, move, calc));
    }
    void check_calc_move_order() {
        // C(n,x)c(y) -> c(y)C(n,x)
        if (insns->size() < 2)
            return;
        Instruction c1 = at(-2), c2 = at(-1);
        if (c1.op != MOVE_CALC || c2.op != CALC)
            return;
        pop(2);
        insns->push_back(c2);
        check_calc();
        insns->push_back(c1);
    }
    void check_move_calc_merge() {
        // C(n,x)C(m,y) -> m(n)c(x)m(m-n)c(y)m(-m)
        if (insns->size() < 2)
            return;
        Instruction c1 = at(-2), c2 = at(-1);
        if (c1.op != MOVE_CALC || c2.op != MOVE_CALC)
            return;
        pop(2);
        int n = c1.value.s2.s0, m = c2.value.s2.s0;
        int x = c1.value.s2.s1, y = c2.value.s2.s1;
        insns->push_back(Instruction(MOVE, n));
        insns->push_back(Instruction(CALC, x));
        if (m - n != 0)
            insns->push_back(Instruction(MOVE, m - n));
        insns->push_back(Instruction(CALC, y));
        insns->push_back(Instruction(MOVE, -m));
    }
    void check_move_calc_move_merge() {
        // C(n,x)m(m) -> m(n)c(x)m(m-n)
        if (insns->size() < 2)
            return;
        Instruction c1 = at(-2), c2 = at(-1);
        if (c1.op != MOVE_CALC || c2.op != MOVE)
            return;
        int n = c1.value.s2.s0;
        int x = c1.value.s2.s1;
        int m = c2.value.i1;
        pop(2);
        insns->push_back(Instruction(MOVE, n));
        insns->push_back(Instruction(CALC, x));
        if (m - n != 0)
            insns->push_back(Instruction(MOVE, m-n));
    }
    void check_mem_move() {
        // [-C(n,x)] -> M(n,x)m(-n)
        if (insns->size() < 4)
            return;
        Instruction c1 = at(-4), c2 = at(-3), c3 = at(-2), c4 = at(-1);
        int val2 = calc_value(c2);
        if (c1.op != OPEN || c2.op != CALC || val2 != -1 || c3.op != MOVE_CALC || c4.op != CLOSE)
            return;
        pop(4);
        short move = c3.value.s2.s0;
        short calc = c3.value.s2.s1;
        push(Instruction(MEM_MOVE, move, calc));
        push(Instruction(MOVE, -move));
    }
    void check_search_zero() {
        // [m(n)] -> s(n)
        if (insns->size() < 3)
            return;
        Instruction c1 = at(-3), c2 = at(-2), c3 = at(-1);
        int val2 = move_value(c2);
        if (c1.op != OPEN || val2 == 0 || c3.op != CLOSE)
            return;
        short move = val2;
        pop(3);
        push(Instruction(SEARCH_ZERO, move));
    }
    void check_fast_loop() {
        // [ ... ] -> { ... }
        //   if loop is innermost and has single loop counter and simple instruction only
        if (insns->size() < 2)
            return;
        if (at(-1).op != CLOSE)
            return;
        int move = 0;
        int i;
        for (i = -2; at(i).op != OPEN; --i) {
            Instruction insn = at(i);
            // has inner loop
            if (insn.op == CLOSE || insn.op == CLOSE_FAST)
                return;
            if (is_undeterminable_move(insn))
                return;
            if (is_ecx_used(insn))
                return;
            move += move_value_for_index_calculation(at(i));
        }
        if (move != 0)
            return;
        for (i = -2; at(i).op != OPEN; --i) {
            Instruction insn = at(i);
            move += move_value_for_index_calculation(at(i));
            if (move == 0 && insn.op == CALC) {
                at(i) = Instruction(CALC_FAST, insn.value);
            }
        }
        at(i) = Instruction(OPEN_FAST);
        if (at(-2).op == MOVE)
            pop(2);
        else
            pop(1);
        insns->push_back(Instruction(CLOSE_FAST));
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
    void push_calc(Opcode op) {
        switch(op) {
        case INC:
            insns->push_back(Instruction(CALC, 1));
            break;
        case DEC:
            insns->push_back(Instruction(CALC, -1));
            break;
        default:
            throw "Unsupported";
        }
        optimizer.check_calc();
        optimizer.check_load();
        optimizer.check_load_dup();
        optimizer.check_calc_move_order();
    }
    void push_move(Opcode op) {
        switch(op) {
        case NEXT:
            insns->push_back(Instruction(MOVE, 1));
            break;
        case PREV:
            insns->push_back(Instruction(MOVE, -1));
            break;
        default:
            throw "Unsupported";
        }
        optimizer.check_move();
        optimizer.check_move_calc();
        optimizer.check_move_calc_merge();
        optimizer.check_move_calc_move_merge();
    }
    void push_next() {
        push_move(NEXT);
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
        pcstack.pop();
        optimizer.check_reset_zero();
        optimizer.check_mem_move();
        optimizer.check_search_zero();
        optimizer.check_fast_loop();
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
                compiler.push_calc(INC);
                break;
            case '-':
                compiler.push_calc(DEC);
                break;
            case '>':
                compiler.push_move(NEXT);
                break;
            case '<':
                compiler.push_move(PREV);
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
        switch(insn.op) {
        case MOVE:
            switch(insn.value.i1) {
            case 1:
                printf(">");
                break;
            case -1:
                printf("<");
                break;
            default:
                printf("%s", name);
            }
            break;
        case CALC:
            switch(insn.value.i1) {
            case 1:
                printf("+");
                break;
            case -1:
                printf("-");
                break;
            default:
                printf("%s", name);
            }
            break;
        case LOAD:
            switch(insn.value.i1) {
            case 0:
                printf("z");
                break;
            default:
                printf("l");
            }
            break;
        default:
            printf("%s", name);
        }
        switch(insn.op) {
            case INC:
            case DEC:
            case NEXT:
            case PREV:
            case GET:
            case PUT:
            case OPEN:
            case OPEN_FAST:
            case CLOSE:
            case CLOSE_FAST:
            case RESET_ZERO:
                break;
            case CALC:
            case MOVE:
                if (verbose && insn.value.i1 != 1 && insn.value.i1 != -1) {
                    printf("(%d)", insn.value.i1);
                }
                break;
            case CALC_FAST:
            case SEARCH_ZERO:
            case LOAD:
                if (verbose && insn.value.i1 != 0) {
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
char* toLabel(char ch, int num) {
    static char labelbuf[BUFSIZ];
    snprintf(labelbuf, sizeof(labelbuf), "%c%d", ch, num);
    return labelbuf;
}
void jit(Xbyak::CodeGenerator &gen, std::vector<Instruction> &insns, int membuf[MEMSIZE]) {
    gen.push(gen.ebx);

    Xbyak::Reg32 memreg = gen.ebx;
    Xbyak::Address mem = gen.dword[memreg];

    gen.mov(memreg, (Xbyak::uint32) membuf);

    std::stack<int> labelStack;
    int labelNum = 0;
    int beginNum;
    int searchNum = 0;
    for (size_t pc=0;;++pc) {
        Instruction insn = insns[pc];
        switch (insn.op) {
            case INC:
                gen.inc(mem);
                break;
            case DEC:
                gen.dec(mem);
                break;
            case NEXT:
                gen.add(memreg, 4);
                break;
            case PREV:
                gen.add(memreg, -4);
                break;
            case GET:
                gen.call((void*) getchar);
                gen.mov(mem, gen.eax);
                break;
            case PUT:
                gen.push(mem);
                gen.call((void*) putchar);
                gen.pop(gen.eax);
                break;
            case OPEN:
                gen.L(toLabel('L', labelNum));
                gen.mov(gen.eax, mem);
                gen.test(gen.eax, gen.eax);
                gen.jz(toLabel('R', labelNum), Xbyak::CodeGenerator::T_NEAR);

                labelStack.push(labelNum);
                ++labelNum;
                break;
            case CLOSE:
                beginNum = labelStack.top();
                labelStack.pop();

                gen.jmp(toLabel('L', beginNum), Xbyak::CodeGenerator::T_NEAR);
                gen.L(toLabel('R', beginNum));
                break;
            case CALC:
                if (insn.value.i1 != 0)
                    gen.add(mem, insn.value.i1);
                break;
            case OPEN_FAST:
                gen.mov(gen.ecx,mem);
                gen.L(toLabel('L', labelNum));
                gen.test(gen.ecx, gen.ecx);
                gen.jz(toLabel('R', labelNum), Xbyak::CodeGenerator::T_NEAR);
                gen.push(memreg);

                labelStack.push(labelNum);
                ++labelNum;
                break;
            case CLOSE_FAST:
                beginNum = labelStack.top();
                labelStack.pop();

                gen.pop(memreg);
                gen.jmp(toLabel('L', beginNum), Xbyak::CodeGenerator::T_NEAR);
                gen.L(toLabel('R', beginNum));
                gen.mov(mem,gen.ecx);
                break;
            case CALC_FAST:
                if (insn.value.i1 != 0)
                    gen.add(gen.ecx, insn.value.i1);
                break;
            case MOVE:
                if (insn.value.i1 != 0)
                    gen.add(memreg, insn.value.i1 * 4);
                break;
            case RESET_ZERO:
                gen.mov(mem, 0);
                break;
            case MOVE_CALC:
                if (insn.value.s2.s1 != 0) {
                    if (insn.value.s2.s0 != 0) {
                        gen.add(memreg, insn.value.s2.s0 * 4);
                        gen.add(mem, insn.value.s2.s1);
                        gen.add(memreg, -insn.value.s2.s0 * 4);
                    } else {
                        gen.add(mem, insn.value.s2.s1);
                    }
                }
                break;
            case MEM_MOVE:
                gen.mov(gen.eax, mem);
                gen.mov(gen.edx, insn.value.s2.s1);
                gen.mul(gen.edx);
                gen.mov(mem, 0);
                gen.add(memreg, insn.value.s2.s0 * 4);
                gen.add(mem, gen.eax);
                break;
            case SEARCH_ZERO:
                gen.mov(gen.eax, insn.value.i1 * 4);
                gen.mov(gen.edx, mem);
                gen.test(gen.edx, gen.edx);
                gen.jz(toLabel('E', searchNum));
                gen.L(toLabel('S', searchNum));
                gen.add(memreg, gen.eax);
                gen.mov(gen.edx, mem);
                gen.test(gen.edx, gen.edx);
                gen.jnz(toLabel('S', searchNum));
                gen.L(toLabel('E', searchNum));
                ++searchNum;
                break;
            case LOAD:
                gen.mov(mem, insn.value.i1);
                break;
            default:
                throw "jit compile error";
            case END:
                gen.pop(gen.ebx);
                gen.ret();
                return;
        }
    }
}
void execute(Xbyak::CodeGenerator &gen) {
    void (*codes)() = (void (*)()) gen.getCode();
    codes();
}
int main(int argc, char *argv[]) {
    static int membuf[MEMSIZE];
    Xbyak::CodeGenerator gen(CODESIZE);
    std::vector<Instruction> insns;
    if(argc == 1) {
        printf("usage: $0 <file>(- for stdin) [-debug[-verbose]]\n");
    } else if(argc >= 2) {
        if (strcmp(argv[1], "-") == 0) {
            parse(insns, stdin);
        } else {
            FILE* file = fopen(argv[1],"r");
            parse(insns, file);
            fclose(file);
        }
        if (argc == 2) {
            jit(gen, insns, membuf);
            execute(gen);
        } else if (argc == 3) {
            const char *option = argv[2];
            if (strcmp(option, "-debug") == 0) {
                debug(insns, false);
            } else if (strcmp(option, "-debug-verbose") == 0) {
                debug(insns, true);
            }
        }
    }
    return 0;
}
