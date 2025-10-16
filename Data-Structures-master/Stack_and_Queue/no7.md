문제->괄호 문자열이 balanced 인지 판단
수도코드

문자를 하나씩 순회
	(,{,[면 스택에 push
	),},]면 스택의 최상단에 맞는 쌍이 있는지 확인 후 pop

스택에 남은게 있으면 not balanced

## expression으로 문자를 하나씩 순회하는 방법
### - while문에서 expression++ 로 다음 문자로 이동후 *expression 으로 확인
```c
while (*expression){//\0의 아스키코드가 0
    if(*expression==answer) return...
    ...
    expression++;
}
```
#### 간결하고 효율적이다
<br>

### - for문에서 i를 선언하고 expression[i]로 확인
```c
for(int i=0;expression[i]!='\0';i++) {
    if(expression[i]==answer) return...
    ...
}
```
#### 가독성이 좋다
<br>

## 여는 괄호 확인
대부분
```c
if (ch == '{' || ch == '[' || ch == '(')
    push...
```
<br>

## 닫는괄호를 매칭하는 방법
### - if-else 문 안에 모든 경우를 peek로 확인
```c
else if (*expression == ')' && peek(&s) == '(' || 
         *expression == ']' && peek(&s) == '[' || 
         *expression == '}' && peek(&s) == '{' )
    pop...
    
```
#### pop 전에 매칭 여부를 미리 검사하여 스택을 건드리지 않는다
<br><br>

### - pop으로 확인
```c
if ( expression[i] == ')') {
    if ( pop(&s) != a ) return 1;
    continue;
}
if ( expression[i] == '}') {
    if ( pop(&s) != b ) return 1;
    continue;
}
if ( expression[i] == ']') {
    if ( pop(&s) != c ) return 1;
    continue;
}
    
```
<br><br>
stack이 비어있을 때 예외처리를 해야할 것 같지만 peek, pop함수에 이미 예외처리가 존재한다
```c
int pop(Stack *s)
{
	int item;
	if (s->ll.head != NULL)
	{
		item = ((s->ll).head)->item;
		removeNode(&(s->ll), 0);
		return item;
	}
	else//NULL이면 -1000반환
		return MIN_INT;//==-1000
}
int peek(Stack *s){
    if(isEmptyStack(s))
        return MIN_INT;
    else
        return ((s->ll).head)->item;
}
```

스택이 비어있을 때 peek, pop해도 MIN_INT((int)-1000)이 반환되서 stack이 비었는지 확인 안해도 제대로 작동한다(아스키코드가 -1000일수는 없으니까)

#### peek, pop 함수는 MIN_INT와 같은 특수한 값을 반환하는 예외 처리에 의존한다 < 효율적이지만 좋은 코드는 아니다
peek나 pop을 하기 전에 isEmptyStack를 먼저 하는게 낫다. 물론 아스키코드가 -1000일수는 없으므로 안해도 문제가 일어나진 않는다.
<br>

### - switch-case문 이용해서 확인
```c
switch (*expr) {
    case '(':
    case '{':
    case '[':
        push(&s, *expr);
        break;
    case ')':
        c = pop(&s);
        if (c != '(') {
            return 1;
        }
        break;
    case '}':
        c = pop(&s);
        if (c != '{') {
            return 1;
        }
        break;
    case ']':
        c = pop(&s);
        if (c != '[') {
            return 1;
        }
        break;
    default:
        return 1;
}
```
#### 코드의 가독성이 높다. 새로운 괄호 쌍을 추가할 때 case 블록만 추가하면 되므로, 유지보수가 용이하고 확장성이 좋다
<br>

### 문자를 아스키코드로 활용
```
문자 리터럴	아스키(ASCII) 코드 값 (10진수)
'('	40
')'	41
'['	91
']'	93
'{'	123
'}'	125
```

### - 아스키코드 배열로 매칭되는 괄호를 확인
```c
char match[128]={0};
match[')']='(';//match[41]='('과 같다
match['}']='{';
match[']']='[';
    char temp=match[expression[i]];//temp = match[')'] == '('
    if(temp!=pop(&stack))
```
#### 배열을 해시 맵(Hash Map)처럼 활용하여 닫는 괄호의 아스키 코드 값을 인덱스로 사용하여 해당되는 여는 괄호를 즉시 찾아내기 때문에 시간복잡도가 가장 좋은 방법이다. 또한 새로운 괄호 쌍을 추가할 때 코드 수정 없이 데이터만 추가하면 되므로, 가장 확장성이 좋고 유지보수가 용이한 방법이다. 


| 방식   |      시간 복잡도      |  실제 연산 과정 (오버헤드) |
|----------|:-------------:|:------|
| if-else if 문 |  O(1) | 순차적인 비교 연산 (최악의 경우 3번의 비교와 논리 연산) |
| switch-case 문 |    O(1)   |  점프 테이블 계산 후 점프 (배열 접근과 비슷하나 아주 미세한 오버헤드 존재 가능) |
| 배열 (Hash Map) 활용 | O(1) | 인덱스 계산 → 메모리 직접 접근 (가장 적음) |


### - 아스키코드의 10으로 나눈 몫으로 확인(?)
```c
if (s_temp.ll.head == NULL && peek(&s_temp) / 10 != *expression / 10)
```
스택에'('(40)가 있는데 다음 문자가 '*'(42),'+' (43) .... 라면?
--문제 조건이 괄호만 입력된다 <라고하면 가능

<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>

```c
int balanced(char *expression)
{
/* add your code here */
	int i=0;
	char match[128]={0};
	match[')']='(';
	match['}']='{';
	match[']']='[';
	Stack stack={{0,NULL}};
	for(int i=0;expression[i]!='\0';i++) {
		if(expression[i]=='('||expression[i]=='['||expression[i]=='{')
			push(&stack, expression[i]);
		else{
			if(isEmptyStack(&stack)){//<----------------------------지워도 됨
				return 1;
			}
			char temp=match[expression[i]];
			if(temp!=pop(&stack)){
				return 1;
			}
		}
	}
	if(!isEmptyStack(&stack)) {
		return 1;
	}
	return 0;
}
```

```c
int balanced(char *expression)
{
	Stack s;

	s.ll.head = NULL;
	s.ll.size = 0;

	if (expression == NULL) return 1;

	while(*expression)
	{
		if(*expression == '(' || *expression == '[' || *expression == '{')
		{
			push(&s, *expression);
		} else
		{
			if (isEmptyStack(&s)) //<---------------------------------------------------------------------- 지워도 됨
			{
				return 1;
			} 
			else if (*expression == ')' && peek(&s) == '(' || *expression == ']' && peek(&s) == '[' || *expression == '}' && peek(&s) == '{' )
			{
				pop(&s);
			} else
			{
				return 1;
			}
		}
		expression++;
	}

	int result = isEmptyStack(&s) ? 0 : 1;
	return result;
}
```


```c
int balanced(char *expression)
{
	Stack s;
	s.ll.head = NULL;
	s.ll.size = 0;

	for (int i = 0; expression[i] != '\0'; i++)
	{
		char temp = expression[i];

		if (temp == '(' || temp == '[' || temp == '{')
		{
			push(&s, temp);
		}
		else if (temp == ')')
		{
			if (peek(&s) == '(')//-------------pop이 나을수도
				pop(&s);
			else
				return 1;
		}
		else if (temp == ']')
		{
			if (peek(&s) == '[')
				pop(&s);
			else
				return 1;
		}
		else if (temp == '}')
		{
			if (peek(&s) == '{')
				pop(&s);
			else
				return 1;
		}
		else
		{
			return 1;
		}
	}

	return 0;
}
```

```c
int balanced(char *expression)
{
    char c;
    Stack s;
    s.ll.head = NULL;
    s.ll.size = 0;

    for (char *expr = expression; *expr != '\0'; expr++) {
        switch (*expr) {
            case '(':
            case '{':
            case '[':
                push(&s, *expr);
                break;
            case ')':
                c = pop(&s);
                if (c != '(') {
                    return 1;
                }
                break;
            case '}':
                c = pop(&s);
                if (c != '{') {
                    return 1;
                }
                break;
            case ']':
                c = pop(&s);
                if (c != '[') {
                    return 1;
                }
                break;
            default:
                return 1;
        }
    }

    if (!isEmptyStack(&s)) {
        return 1;
    }
    return 0;
}
```

```c
int balanced(char *expression)
{
	Stack *myStack = malloc(sizeof(Stack));//-----굳이?

	while (*expression)
	{	
		if (*expression == '(' || *expression == '{' || *expression == '['){
			push(myStack, *expression);
		} 
		else {
			if (isEmptyStack(myStack)) {
				return 1;
			} 
			if (*expression == ')' && peek(myStack) == '(' || *expression == '}' && peek(myStack) == '{' || *expression == ']' && peek(myStack) == '['){
				pop(myStack);
			} 
			else {
				return 1;
			}
		}
		expression++;
	}

	int result;

	if (isEmptyStack(myStack)){
		result = 0;
	} else {
		result = 1;
	}

	free(myStack);

	return result;
}
```

```c
int balanced(char *expression)
{
	Stack s;
	s.ll.head = NULL;
	s.ll.size = 0;
	for (int i = 0; expression[i] != '\0'; i++){
		char ch = expression[i];
		if (ch == '{' || ch == '[' || ch == '('){
			push(&s, ch);
		}
		else{
			if (ch == '}' || ch == ']' || ch == ')'){
				if(isEmptyStack(&s)){
					return 1;
				}
				char top = peek(&s);//<-----------pop이 나을수도?
				if ((ch == ')' && top == '(') ||
					(ch == '}' && top == '{') ||
					(ch == ']' && top == '[')){
						pop(&s);//이거 없애고
				}else{
					return 1;
				}
			}
		}
	}
	if (isEmptyStack(&s)) return 0; else return 1;
}
```

```c
int balanced(char *expression)
{
	Stack s;
	s.ll.head = NULL;
	s.ll.size = 0;

	for (int i = 0; i < strlen(expression); i++) {
		char ch = expression[i];

		if (ch == '(' || ch == '{' || ch == '[') {
			push(&s, ch);
		} else {
			if (isEmptyStack(&s)) {
				return 1;
			}

			int top = peek(&s);//마찬가지로 pop??

			if ((top == '(' && ch == ')') || (top == '{' && ch == '}') || (top == '[' && ch == ']')) {
				pop(&s);
			} else {
				return 1;
			}
		}
	}

	return isEmptyStack(&s) ? 0 : 1;
}
```

```c
int balanced(char *expression)
{
    
    Stack s;
    s.ll.head = NULL;
    s.ll.size = 0;

    int a = '(';
    int b = '{';
    int c = '[';

    for ( int i = 0; expression[i] != '\0'; i++ ) {
        if ( expression[i] == ')') {
            if ( pop(&s) != a ) return 1;
            continue;
        }
        if ( expression[i] == '}') {
            if ( pop(&s) != b ) return 1;
            continue;
        }
        if ( expression[i] == ']') {
            if ( pop(&s) != c ) return 1;
            continue;
        }

        push(&s,expression[i]);
    }

    return 0;
}
```

```c
int balanced(char *expression)
{
	Stack s_temp;

	s_temp.ll.head = NULL;
	s_temp.ll.size = 0;
	while (*expression)
	{
		if (*expression == '(' || *expression == '[' || *expression == '{')
			push(&s_temp, *expression);
		else if (*expression == ')' || *expression == ']' || *expression == '}')
			if (s_temp.ll.head == NULL && peek(&s_temp) / 10 != *expression / 10)//?
				return 1;
			else
				pop(&s_temp);
		else
			return 1;
		expression++;
	}
	if (!isEmptyStack(&s_temp))
	{
		removeAllItemsFromStack(&s_temp);
		return 1;
	}
	else
		return 0;
}
```