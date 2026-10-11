#include <iostream>
#include <ctime>

typedef unsigned long long msize;
const msize one = 1;

struct Node {
    char op;
    int left, right, var;
};

struct Expression {
    const char *text;
    const char *error;
    int pos, count, root;
    Node nodes[256];
};

struct Result {
    msize checked;
    msize values[26], left, right;
    bool equal;
};

bool same_text(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

void copy_values(msize to[26], const msize from[26]) {
    for (int i = 0; i < 26; i++) to[i] = from[i];
}

void spaces(Expression *e) {
    while (e->text[e->pos] == ' ' || e->text[e->pos] == '\t') e->pos++;
}

int add_node(Expression *e, char op, int left, int right, int var) {
    if (e->count == 256) {
        e->error = "слишком длинное выражение";
        return -1;
    }
    int i = e->count++;
    e->nodes[i].op = op;
    e->nodes[i].left = left;
    e->nodes[i].right = right;
    e->nodes[i].var = var;
    return i;
}

int parse_union(Expression *e);

int parse_atom(Expression *e) {
    spaces(e);
    char c = e->text[e->pos];
    if (c == '~' || c == '!') {
        e->pos++;
        int a = parse_atom(e);
        return a < 0 ? -1 : add_node(e, '!', a, -1, -1);
    }
    if (c == '(') {
        e->pos++;
        int a = parse_union(e);
        spaces(e);
        if (e->text[e->pos] != ')') {
            e->error = "нет закрывающей скобки";
            return -1;
        }
        e->pos++;
        return a;
    }
    if (c >= 'A' && c <= 'Z') {
        e->pos++;
        return add_node(e, 'v', -1, -1, c - 'A');
    }
    e->error = "ожидалась переменная или '('";
    return -1;
}

int parse_intersection(Expression *e) {
    int left = parse_atom(e);
    while (left >= 0) {
        spaces(e);
        char op = e->text[e->pos];
        if (op == '~' && e->text[e->pos + 1] == '&') {
            op = '\\';
            e->pos += 2;
        } else if (op == '&' || op == '\\') {
            e->pos++;
        } else {
            break;
        }
        int right = parse_atom(e);
        left = right < 0 ? -1 : add_node(e, op, left, right, -1);
    }
    return left;
}

int parse_xor(Expression *e) {
    int left = parse_intersection(e);
    while (left >= 0) {
        spaces(e);
        if (e->text[e->pos] != '^') break;
        e->pos++;
        int right = parse_intersection(e);
        left = right < 0 ? -1 : add_node(e, '^', left, right, -1);
    }
    return left;
}

int parse_union(Expression *e) {
    int left = parse_xor(e);
    while (left >= 0) {
        spaces(e);
        if (e->text[e->pos] != '|') break;
        e->pos++;
        int right = parse_xor(e);
        left = right < 0 ? -1 : add_node(e, '|', left, right, -1);
    }
    return left;
}

bool parse(Expression *e, const char *text) {
    e->text = text;
    e->error = 0;
    e->pos = 0;
    e->count = 0;
    e->root = -1;
    int length = 0;
    while (text[length]) length++;
    if (length > 255) {
        e->error = "слишком длинное выражение";
        return false;
    }
    e->root = parse_union(e);
    spaces(e);
    if (e->root >= 0 && e->text[e->pos]) e->error = "лишний символ";
    return e->root >= 0 && !e->error;
}

msize calculate(const Expression *e, int i, const msize values[26], msize universe) {
    const Node *node = &e->nodes[i];
    if (node->op == 'v') return values[node->var];
    msize a = calculate(e, node->left, values, universe);
    if (node->op == '!') return universe & ~a;
    msize b = calculate(e, node->right, values, universe);
    if (node->op == '&') return a & b;
    if (node->op == '|') return a | b;
    if (node->op == '^') return a ^ b;
    return a & ~b;
}

int variable_list(const Expression *a, const Expression *b, int list[26]) {
    bool used[26] = {};
    for (int i = 0; i < a->count; i++)
        if (a->nodes[i].op == 'v') used[a->nodes[i].var] = true;
    for (int i = 0; i < b->count; i++)
        if (b->nodes[i].op == 'v') used[b->nodes[i].var] = true;
    int count = 0;
    for (int i = 0; i < 26; i++) if (used[i]) list[count++] = i;
    return count;
}

void print_set(msize set, int n) {
    std::cout << '{';
    bool first = true;
    for (int i = 0; i < n; i++) {
        if (set & (one << i)) {
            if (!first) std::cout << ", ";
            std::cout << i + 1;
            first = false;
        }
    }
    std::cout << '}';
}

int cardinality(msize set) {
    int count = 0;
    while (set) {
        count += set & 1;
        set >>= 1;
    }
    return count;
}

bool is_subset(msize a, msize b) {
    return (a & b) == a;
}

void Boolean(const msize *set, int n) {
    if (!set || n < 0 || n > 16) return;
    for (msize mask = 0; mask < (one << n); mask++) {
        std::cout << '{';
        bool first = true;
        for (int i = 0; i < n; i++) {
            if (mask & (one << i)) {
                if (!first) std::cout << ", ";
                std::cout << set[i];
                first = false;
            }
        }
        std::cout << "} ";
    }
    std::cout << '\n';
}

Result check_fast(const Expression *a, const Expression *b, int n) {
    Result result = {};
    result.equal = true;
    int list[26];
    int k = variable_list(a, b, list);
    msize total = n == 0 ? 1 : one << k;
    for (msize mask = 0; mask < total; mask++) {
        msize values[26] = {};
        for (int i = 0; i < k; i++)
            if (mask & (one << i)) values[list[i]] = 1;
        msize left = calculate(a, a->root, values, n == 0 ? 0 : 1);
        msize right = calculate(b, b->root, values, n == 0 ? 0 : 1);
        result.checked++;
        if (left != right) {
            result.equal = false;
            copy_values(result.values, values);
            result.left = left;
            result.right = right;
            break;
        }
    }
    return result;
}

Result check_slow(const Expression *a, const Expression *b, int n) {
    Result result = {};
    result.equal = true;
    int list[26];
    int k = variable_list(a, b, list);
    msize total = one << (k * n);
    msize universe = n == 64 ? ~msize(0) : (one << n) - 1;
    for (msize mask = 0; mask < total; mask++) {
        msize values[26] = {};
        for (int i = 0; i < k; i++)
            values[list[i]] = (mask >> (i * n)) & universe;
        msize left = calculate(a, a->root, values, universe);
        msize right = calculate(b, b->root, values, universe);
        result.checked++;
        if (left != right) {
            result.equal = false;
            copy_values(result.values, values);
            result.left = left;
            result.right = right;
            break;
        }
    }
    return result;
}

void print_result(const Expression *a, const Expression *b, int n, Result result) {
    std::cout << (result.equal ? "ЗАКОН" : "ОПЕЧАТКА")
              << "; проверено: " << result.checked << '\n';
    if (result.equal) return;
    int list[26];
    int k = variable_list(a, b, list);
    for (int i = 0; i < k; i++) {
        std::cout << char('A' + list[i]) << " = ";
        print_set(result.values[list[i]], n);
        std::cout << '\n';
    }
    std::cout << "левая часть = ";
    print_set(result.left, n);
    std::cout << ", правая часть = ";
    print_set(result.right, n);
    std::cout << '\n';
}

int check(const char *left, const char *right, int n, bool slow) {
    Expression a, b;
    if (!parse(&a, left)) {
        std::cerr << "Левая часть: " << a.error << ", позиция " << a.pos + 1 << '\n';
        return 1;
    }
    if (!parse(&b, right)) {
        std::cerr << "Правая часть: " << b.error << ", позиция " << b.pos + 1 << '\n';
        return 1;
    }
    int list[26];
    int k = variable_list(&a, &b, list);
    if (slow && k * n >= 64) {
        std::cerr << "Слишком много присваиваний для полного перебора.\n";
        return 1;
    }
    Result result = slow ? check_slow(&a, &b, n) : check_fast(&a, &b, n);
    print_result(&a, &b, n, result);
    return 0;
}

int demo() {
    msize a = 11, b = 6;
    std::cout << "A = "; print_set(a, 4);
    std::cout << ", B = "; print_set(b, 4);
    std::cout << "\nA | B = "; print_set(a | b, 4);
    std::cout << ", A & B = "; print_set(a & b, 4);
    std::cout << ", A \\ B = "; print_set(a & ~b, 4);
    std::cout << ", A ^ B = "; print_set(a ^ b, 4);
    std::cout << "\n|A| = " << cardinality(a)
              << ", A включено в B: " << (is_subset(a, b) ? "да" : "нет") << '\n';
    msize set[3] = {1, 2, 3};
    std::cout << "P({1, 2, 3}): ";
    Boolean(set, 3);

    const char *left[] = {"~(A|B)", "A&(B|C)", "~~A", "A\\B", "A^B", "(A\\B)\\C"};
    const char *right[] = {"~A&~B", "(A&B)|(A&C)", "A", "A", "B^A", "A\\(B\\C)"};
    bool expected[] = {true, true, true, false, true, false};
    for (int i = 0; i < 6; i++) {
        Expression x, y;
        parse(&x, left[i]);
        parse(&y, right[i]);
        if (check_fast(&x, &y, 3).equal != expected[i] ||
            check_slow(&x, &y, 3).equal != expected[i]) {
            std::cerr << "Ошибка проверки примера " << i + 1 << '\n';
            return 1;
        }
        std::cout << '\n' << left[i] << " == " << right[i] << '\n';
        print_result(&x, &y, 6, check_fast(&x, &y, 6));
    }
    return 0;
}

void benchmark() {
    Expression a, b;
    parse(&a, "(A&B)|(C&D)");
    parse(&b, "(B&A)|(D&C)");
    std::cout << "n | быстрый (мкс) | полный (мс) | присваиваний\n";
    for (int n = 1; n <= 8; n++) {
        volatile msize checked = 0;
        std::clock_t start = std::clock();
        for (int i = 0; i < 100000; i++) checked += check_fast(&a, &b, n).checked;
        double fast = 1000000.0 * (std::clock() - start) / CLOCKS_PER_SEC / 100000;
        start = std::clock();
        Result slow = check_slow(&a, &b, n);
        double full = 1000.0 * (std::clock() - start) / CLOCKS_PER_SEC;
        std::cout << n << " | " << fast << " | " << full << " | " << slow.checked << '\n';
        if (!slow.equal || checked != 100000 * (one << 4)) {
            std::cerr << "Ошибка при замере\n";
            return;
        }
    }
}

bool read_n(const char *text, int *n) {
    if (!*text) return false;
    int value = 0;
    for (int i = 0; text[i]; i++) {
        if (text[i] < '0' || text[i] > '9') return false;
        value = value * 10 + text[i] - '0';
        if (value > 64) return false;
    }
    *n = value;
    return true;
}

int main(int argc, char **argv) {
    if (argc == 1 || (argc == 2 && same_text(argv[1], "--demo"))) return demo();
    if (argc == 2 && same_text(argv[1], "--bench")) {
        benchmark();
        return 0;
    }
    bool slow = argc == 5 && same_text(argv[1], "--slow");
    if ((!slow && argc != 4) || (argc == 5 && !slow)) {
        std::cerr << "Запуск: " << argv[0] << " [--slow] 'A|B' 'B|A' n\n";
        std::cerr << "Операции: ~, &, \\, ^, |; переменные A-Z; n от 0 до 64.\n";
        return 1;
    }
    int offset = slow ? 1 : 0;
    int n;
    if (!read_n(argv[3 + offset], &n)) {
        std::cerr << "n должно быть от 0 до 64.\n";
        return 1;
    }
    return check(argv[1 + offset], argv[2 + offset], n, slow);
}
