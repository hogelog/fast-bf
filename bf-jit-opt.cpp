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
    CALC, MOVE,
    SEARCH_ZERO, LOAD,
    SET_MULTIPLIER, CALC_MULT
};
const char *OPCODE_NAMES[] = {
    "+", "-", ">", "<",
    ",", ".", "[", "]", "",
    "c", "m",
    "s", "l",
    "X", "x",
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
    void check_multiplier_loop() {
        // [A] -> Xl(0)A'
        //   when A contains ><+- only, and >< is balanced, and p[0] decreased by 1
        //   where A' = replace c(n) to x(n), and remove p[0]-- from A.
        if (insns->size() < 4)
            return;
        if (at(-1).op != CLOSE)
            return;
        int loop_start=-2;
        for (; at(loop_start).op != OPEN; --loop_start)
            ;
        int move = 0;
        int counter_delta = 0;
        for (int i = loop_start + 1; i < -1; ++i) {
            if (at(i).op != MOVE && at(i).op != CALC)
                return;
            move += move_value(at(i));
            if (move == 0)
                counter_delta += calc_value(at(i));
        }
        if (move != 0)
            return;
        if (counter_delta != -1)
            return;

        std::vector<Instruction> new_ops;
        new_ops.push_back(Instruction(SET_MULTIPLIER));
        new_ops.push_back(Instruction(LOAD,0));
        move = 0;
        for (int i = loop_start + 1; i < -1; ++i) {
            move += move_value(at(i));
            if (at(i).op == CALC) {
                if (move != 0)
                    new_ops.push_back(Instruction(CALC_MULT,at(i).value.i1));
            } else {
                new_ops.push_back(at(i));
            }
        }

        pop(-loop_start);
        for (std::vector<Instruction>::iterator it = new_ops.begin(); it != new_ops.end(); ++it) {
            push(*it);
        }
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
        optimizer.check_search_zero();
        optimizer.check_multiplier_loop();
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
        printf("%s", name);
        switch(insn.op) {
            case INC:
            case DEC:
            case NEXT:
            case PREV:
            case GET:
            case PUT:
            case OPEN:
            case CLOSE:
            case SET_MULTIPLIER:
                break;
            case CALC:
            case CALC_MULT:
            case MOVE:
                if (verbose) {
                    printf("(%d)", insn.value.i1);
                }
                break;
            case SEARCH_ZERO:
            case LOAD:
                if (verbose) {
                    printf("(%d)", insn.value.i1);
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
            case MOVE:
                if (insn.value.i1 != 0)
                    gen.add(memreg, insn.value.i1 * 4);
                break;
            case SET_MULTIPLIER:
                gen.mov(gen.edx, mem);
                break;
            case CALC_MULT:
                gen.mov(gen.eax, insn.value.i1);
                gen.imul(gen.eax,gen.edx);
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
            case END:
                gen.pop(gen.ebx);
                gen.ret();
                return;
            default:
                throw "jit compile error";
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
