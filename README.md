## CHR3.5 语法

### 基本语法

```
arith_expr ::= int
             | aith_expr op arith_expr
op         ::= + | - | * | /

comp_expr ::= arith_epxr cmp arith_expr
cmp       ::= < | = | >

bool      ::= "true" | "false"
lambda    ::= \ id . expr
let_expr  ::= "let" ( bind+ ) expr
bind      ::= id : expr
call      ::= expr expr
cond_expr ::= "cond" ( cond_result_expr+ )
cond_res_expr ::= expr ? epxr
cons      ::= "nil"
            | arith_expr . cons
epxr ::= id | bool | (lambda) | (let_expr) | (call) | (cons)
       | (cond_expr) | (arith_expr) | (cond_expr)
```

每行一个 expr 或者 bind

### 括号

1. `(int)` 和 `(bool)` 的情况下，括号必须省略
1. 非并列关系中的最后一个词法元素的括号必须省略，如 `id : (3+4)` 必须变成 `id : 3+4`，但 `let ((x:1)(y:2)) (x+y)` 中的 `y:2` 的括号就属于并列关系，不能省略，因此变成 `let ((x:1)(y:2)) x+y`
2. 如果一个表达式构成一行，则必须省略最外层的括号
3. cond和let语句中，可以在并列关系外层括号开始的地方换行。如 `let ((x:1)(y:2)) x+y` 根据规则3和4，可以变成

```
let (
    x:1
    y:2
) x+y
```

### 例子

```
1
1 + 2
2 * 3
2 * (3 + 4)
(1 + 2) * (3 + 4)

(\ x . 2 * x) 3

let (
    x : 2
) let (
    f : \ y . x * y
) f 3

let (
    x : 2
) let (
    f : \ y . x * y
) let (
    x:4
) f 3

fact : \n. cond (
    (n=0)? 1
    else?  n*(fact(n-1))
)
fact 5

fib : \n. cond (
    (n<2)? n
    else?  (fib(n-1)) + (fib(n-2))
)
fib 9

even : \n. cond (
    (n=0)? true
    (n=1)? false
    else?  odd (n-1)
)
odd :  \n. cond (
    (n=0)? false
    (n=1)? true
    else?  even(n-1)
)
even 42

nil
1 . nil
2 . 1 . nil
x : 2 . nil
y : 1.x
head x
rest x
null x
null nil
pair x
pair 1

len: \xs.cond (
    (null xs)? 0
    (pair xs)? 1+(len(rest xs))
    else? 1
)
list: 1 . 2 . 3 . 4 . nil
len list
```
