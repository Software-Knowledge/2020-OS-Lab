
section .data
string: db "Please input x and y:", 0Ah
length: equ $-string
line: db "", 0Ah
lineLength: equ $-line
Zero: db "0"
ZeroLength: equ $-Zero
end: db 0

section .bss
num1: resw 20
num2: resw 20

flag1: resb 1
flag2: resb 1

addMulDigit: resw 40
mulDigitResult: resw 40
addResult: resw 40
mulResult: resw 40

num1Place: resb 4
num2Place: resb 4
add1Index: resb 4
add2Index: resb 4

cnt1: resb 4
cnt2: resb 4
mulOneCnt: resb 4
mulDigitCarry: resb 4
nowPlace: resb 4
zeroCnt: resb 4
nowLength: resb 4
fillZeroCnt: resb 4 
mulCnt: resb 4
movCnt: resb 4

index1: resb 4
index2: resb 4
add3Index: resb 4
mulDigitIndex: resb 4

printsize: resb 4
printCnt: resb 4
print: resb 4

isZero: resb 1
digit: resb 1
mulOneDigit: resb 1

section .text
global main:

main:
	mov eax, 4
	mov ebx, 1
	mov ecx, string
	mov edx, length
	int 80h

	call getNumbers

	pusha
	mov dword[num1Place], num1
	mov dword[num2Place], num2
	mov eax, dword[index1]
	mov dword[add1Index], eax
	mov eax, dword[index2]
	mov dword[add2Index], eax
	call addition
	mov dword[num1Place], 0
	mov dword[num2Place], 0
	mov dword[add1Index], 0
	mov dword[add2Index], 0
	popa

	pusha
	mov eax, addResult
	mov ebx, dword[add3Index]
	mov dword[printsize], ebx
	call reverseOutput
	popa

	call multiply

	pusha
	mov eax, mulResult
	mov ebx, dword[nowLength]
	mov dword[printsize], ebx
	call strOutput
	popa

	mov eax, 1
    mov ebx, 0
    int 80h

; --------------------------------------
; 获取两个输入字符
; output:
; num1 第一个数字的首地址
; index1 第一个数字的长度
; flag1 第一个符号，0为正
; num2 第二个数字的首地址
; index2 第二个数字的长度
; flag2 第二个符号，0为正
; 
getNumbers:
	pusha
	mov dword[index1], 0
	mov byte[flag1], 0

	; 一位一位获取
	getNum1:
		; OPCODE 3
		mov eax, 3
		; stdin
		mov ebx, 0
		; 获取的输入放置到digit
		mov ecx, digit
		; 获取1位输入
		mov edx, 1
		int 80h
		
		; 空格为终止符
		cmp byte[digit], " "
		je initNum2

		; 将获取的数字放置到指定的位置
		mov eax, num1
		add eax, dword[index1]
		mov cl, byte[digit]
		mov byte[eax], cl

		inc dword[index1]

		jmp getNum1

	initNum2:
		mov dword[index2], 0
		mov byte[digit], 0
		mov byte[flag2], 0

	getNum2:
		mov eax, 3
		mov ebx, 0
		mov ecx, digit
		mov edx, 1
		int 80h

		; 回车符为终止符
		cmp byte[digit], 0Ah
		je getNumberFinished

		mov eax, num2
		add eax, dword[index2]
		mov cl, byte[digit]
		mov byte[eax], cl
		inc dword[index2]

		jmp getNum2

getNumberFinished:
	popa
	ret

; --------------------------------------
; 完成乘法
; input:
; num2 第二个数字的地址
; index2 第二个输入数字的长度
; 
; process:
; movCnt 移位计数器
; mulCnt 按位乘法计数器
; addMulDigit 乘一位的结果的调整结果
; nowPlace 当前位置
; addResult 加法结果
; 
; output:
; nowLength 结果长度
; mulResult 乘法结果
; 
multiply:
	pusha
	mov eax, dword[index2]
	mov dword[mulCnt], eax

	mov dword[nowPlace], 0
	mov dword[movCnt], 0

	mov dword[mulResult], 0
	mov dword[nowLength], 0

	; 清空所有位置上的值
	pusha
	mov eax, mulResult
	mov ebx, 40
	call fillZero
	popa

	pusha
	mov eax, addResult
	mov ebx, 40
	call fillZero
	popa

	pusha
	mov eax, mulDigitResult
	mov ebx, 40
	call fillZero
	popa

	pusha
	mov eax, addMulDigit
	mov ebx, 40
	call fillZero
	popa

multiplyLoop:
	cmp dword[mulCnt], 0
	je multiplyFinished

	; 执行一位乘法
	mov ebx, num2
	add ebx, dword[mulCnt]
	dec ebx
	mov ecx, dword[ebx]
	sub ecx, 48
	mov byte[mulOneDigit], cl
	pusha
	call multiplyOneDigit
	popa

	call reverse

	mulAddition:
		; 执行加法
		pusha
		mov dword[num1Place], addMulDigit
		mov dword[num2Place], mulResult

		mov eax, dword[mulDigitIndex]
		mov dword[add1Index], eax

		mov eax, dword[nowLength]
		mov dword[add2Index], eax

		call addition 

		mov dword[num1Place], 0
		mov dword[num2Place], 0
		mov dword[add1Index], 0
		mov dword[add2Index], 0
		popa

		mov eax, dword[add3Index]
		mov dword[nowLength], eax
		mov dword[movCnt], eax
	
	; 移动addResult的值到mulResult中去
	mov2Result:
		cmp dword[movCnt], -1
		je mov2ResultFinished

		mov eax, mulResult
		mov ebx, addResult

		sub eax, dword[movCnt]
		add eax, dword[nowLength]
		sub eax, 1

		add ebx, dword[movCnt]
		
		mov cl, byte[ebx]
		mov byte[eax], cl

		dec dword[movCnt]

		jmp mov2Result

	; 转移结束
	mov2ResultFinished:
		mov dword[movCnt], 0
		mov dword[mulDigitIndex], 0
		dec dword[mulCnt]
		inc dword[nowPlace]

		jmp multiplyLoop

multiplyFinished:
	popa
	ret

; --------------------------------------
; 一个数 乘以 一个数字
; input:
; nowPlace 需要在最后补充的0的个数
; index1 第一个输入数字的长度
; 
; process:
; mulOneCnt 第一个输入的数字的长度
; mulDigitCarry 本过程中的每一位的进位的数量
; zeroCnt 需要在最后补充的0的个数
; 
; output:
; mulDigitResult 一个数 乘以 一个数字的结果
; mulDigitIndex 计算结果的位数
; 
; 
multiplyOneDigit:
	mov eax, dword[index1]
	mov dword[mulOneCnt], eax

	mov eax, dword[nowPlace]
	mov dword[zeroCnt], eax

	mov dword[mulDigitCarry], 0
	mov dword[mulDigitIndex], 0
	mov dword[mulDigitResult], 0

	; 左移位
	movLeft:
		cmp dword[zeroCnt], 0
		je multiplyOneDigitLoop

		mov eax, mulDigitResult
		add eax, dword[mulDigitIndex]
		mov cl, 48
		mov byte[eax], cl

		inc dword[mulDigitIndex]
		dec dword[zeroCnt]

		jmp movLeft
	
	; 按位乘运算
	multiplyOneDigitLoop:
		cmp dword[mulOneCnt], 0
		je multiplyOneDigitFinished

		mov ebx, num1
		add ebx, dword[mulOneCnt]
		dec ebx

		; 将num1对应位的值放到eax(al)中
		mov eax, 0
		mov al, byte[ebx]
		sub eax, 48
		mul byte[mulOneDigit]

		; 乘法完成后添加进位
		mov ecx, eax
		add ecx, dword[mulDigitCarry]
		mov dword[mulDigitCarry], 0

	; 获取进位的值
	mulOverflow:
		; 检查进位
		cmp ecx, 10
		jb overflowFinished

		sub ecx, 10
		inc dword[mulDigitCarry]

		jmp mulOverflow

	; 完成进位
	overflowFinished:	
		mov eax, mulDigitResult
		add eax, dword[mulDigitIndex]
		add cl, 48
		mov byte[eax], cl

		inc dword[mulDigitIndex]
		dec dword[mulOneCnt]

		jmp multiplyOneDigitLoop

multiplyOneDigitFinished:
	;检查高位进位
	cmp dword[mulDigitCarry], 0
	je multiplyOneDigitReslt

	mov eax, mulDigitResult
	add eax, dword[mulDigitIndex]
	mov cl, byte[mulDigitCarry]
	add cl, 48
	mov byte[eax], cl

	inc dword[mulDigitIndex]

multiplyOneDigitReslt:
	; 清空进位值
	mov dword[mulDigitCarry], 0
	ret

; ---------------------------------
; 加法 
; Input:
; num1Place 第一个加数的首地址
; add1Index 第一个加数的长度
; num2Place 第二个加数的首地址
; add2Index 第二个加数的长度
;
; Process:
;  ebx 进位结果
;  cnt1 第一个加数计数器
;  cnt2 第二个加数计数器
;
; Output:
; add3Index 加法结果的长度
; 
addition:
	; 清空进位
	mov ebx, 0

	; 使用cnt1和cnt2存储第一个第二个加数长度
	mov eax, dword[add1Index]
	dec eax
	mov dword[cnt1], eax

	mov eax, dword[add2Index]
	dec eax
	mov dword[cnt2], eax

	; 清空结果长度
	mov dword[add3Index], 0

	; 循环按位加法
	additionloop:
		; cnt1 == -1
		cmp dword[cnt1], -1
		je cnt2Longer

		; cnt2 == -1
		cmp dword[cnt2], -1
		je cnt1Longer

		; dh 获取第一个加数数字
		mov eax, dword[num1Place]
		add eax, dword[cnt1]
		mov dh, byte[eax]
		sub dh, 48

		; dl 获取第二个加数数字
		mov eax, dword[num2Place]
		add eax, dword[cnt2]
		mov dl, byte[eax]
		sub dl, 48

		; 如果上一次有进位则需要+1，并且置空ebx
		cmp ebx, 0
		je compare

		inc dh
		mov ebx, 0

		compare:
			; 进行两位的加法
			add dh, dl

			; 检查是否产生进位
			cmp dh, 10
			jl nothigher
			; 如果产生进位
			mov ebx, 1
			sub dh, 10

		nothigher:
			; 没有产生进位
		
			; 换为输出的情况
			add dh, 48

			; 将结果输入到addResult对应的位置中去
			mov eax, addResult
			add eax, dword[add3Index]
			; eax 即将放入的位置
			mov byte[eax], dh
			inc dword[add3Index]

			; 调整两个计数器的长度
			dec dword[cnt1]
			dec dword[cnt2]
			jmp additionloop

	; 第一个数比较长
	cnt1Longer:
		; 判断第一个数是否完全拷贝到结果中去
		cmp dword[cnt1], -1
		je additionFinished 

		; dl 第一个数的对应位置的数字
		mov eax, dword[num1Place]
		add eax, dword[cnt1]
		mov dl, byte[eax]
	
		; 检查上一位是否有溢出
		cmp ebx, 0
		je cnt1LongerNotFlow
		
		inc dl
		mov ebx, 0

		; 注意和48 + 10比较
		cmp dl, 58
		jl cnt1LongerNotFlow

		sub dl, 10
		mov ebx, 1

	cnt1LongerNotFlow:
		; 将当前dl中的数字放到对应位置上
		mov eax, addResult 
		add eax, dword[add3Index] 
		mov byte[eax], dl

		inc dword[add3Index]
		dec dword[cnt1]
		jmp cnt1Longer

	; 第二个数字比较长
	cnt2Longer:
		; 判断第一个数是否完全拷贝到结果中去
		cmp dword[cnt2], -1
		je additionFinished

		; dl 第二个数的对应位置的数字
		mov eax, dword[num2Place]
		add eax, dword[cnt2]
		mov dl, byte[eax]

		; 检查上一位是否有溢出
		cmp ebx, 0
		je cnt2LongerNotFlow

		inc dl
		mov ebx, 0

		; 注意和48 + 10比较
		cmp dl, 58
		jl cnt2LongerNotFlow

		sub dl, 10
		mov ebx, 1

	cnt2LongerNotFlow:
		; 将当前dl中的数字放到对应位置上
		mov eax, addResult
		add eax, dword[add3Index]
		mov byte[eax], dl

		inc dword[add3Index]
		dec dword[cnt2]
		jmp cnt2Longer

additionFinished:
	; 检查最后一位是否产生了进位
	cmp ebx, 0
	je notOverflow

	; 产生了进位，向上进位一个
	mov eax, addResult 
	add eax, dword[add3Index] 
	mov byte[eax], 31h
	inc dword[add3Index]

	notOverflow:
		; 没有进位，清空退出
		mov dword[cnt1], 0
		mov dword[cnt2], 0
		mov dword[num1Place], 0
		mov dword[num2Place], 0
		ret

; --------------------------------------
; 从高位进行输出
; input:
; eax 输出的数字的首地址
; printsize 要打印出来的长度
; 
; process:
; isZero 当前打印的是不是0
; print 即将被打印的数字
; printCnt 计数器
;
; output:
; 输出到控制台
; 
reverseOutput:
	add eax, dword[printsize]

	push eax
	mov byte[isZero], 0
	mov dword[printCnt], 0
	mov dword[print], 0
	dec dword[printsize]

reverseNextchar:
	pop eax
	dec eax
	push eax

	mov dword[print], eax

	cmp byte[isZero], 1
	je reverseNotZero
	
reverseIsZero:
	mov edx, dword[print]
	mov cl, byte[edx]
	cmp cl, 48
	je reverseNextLoop

reverseNotZero:

	mov eax, 4 ; opcode 4
	mov ebx, 1 ; stdout
	mov ecx, dword[print]
	mov edx, 1
	int 80h

	mov byte[isZero], 1

reverseNextLoop:
	mov eax, dword[printsize]

	cmp dword[printCnt], eax
	je reverseOutputFinished

	; reverse to output, inc 1
	inc dword[printCnt]

	jmp reverseNextchar

reverseOutputFinished:
	cmp byte[isZero], 1
	je reverseNotZeroResult

	mov eax, 4
	mov ebx, 1
	mov ecx, Zero
	mov edx, ZeroLength
	int 80h

	; 打印一个 0
reverseNotZeroResult:

	pop eax
	mov eax, 4
	mov ebx, 1
	mov ecx, line
	mov edx, lineLength
	int 80h

	mov byte[isZero], 0

	ret

; --------------------------------------
; 从低位进行输出
; input:
; eax 输出的数字的首地址
; printsize 要打印出来的长度
; 
; process:
; isZero 是否为0
; print 即将被打印的数字
; printCnt 计数器
;
; output:
; 输出到控制台
; 
strOutput:
	dec eax

	push eax

	mov byte[isZero], 0
	mov dword[printCnt], 0
	mov dword[print], 0
	dec dword[printsize]

strNextchar:
	pop eax
	inc eax
	push eax

	mov dword[print], eax

	cmp byte[isZero], 1
	je strNotZero
	
strIsZero:
	mov edx, dword[print]
	mov cl, byte[edx]
	cmp cl, 48
	je strNextLoop

strNotZero:
	mov eax, 4
	mov ebx, 1
	mov ecx, dword[print]
	mov edx, 1
	int 80h
	mov byte[isZero], 1

strNextLoop:
	mov eax, dword[printsize]

	cmp dword[printCnt], eax
	je strOutputFinished

	inc dword[printCnt]

	jmp strNextchar

strOutputFinished:
	cmp byte[isZero], 1
	je strNotZeroResult

	mov eax, 4
	mov ebx, 1
	mov ecx, Zero
	mov edx, ZeroLength
	int 80h

	; 打印一个 0

strNotZeroResult:
	pop eax
	; 输出换行符
	mov eax, 4
	mov ebx, 1
	mov ecx, line
	mov edx, lineLength
	int 80h

	mov byte[isZero], 0

	ret

; ---------------------------------
; 将eax对应地址开始的ebx长度的位置全部填充0
; input:
; eax 存放首地址
; ebx 存放数据长度
; 
; process:
; fillZeroCnt: 计数器
fillZero:
	; 初始化计数器
	; 将首地址存放到ecx中去
	mov dword[fillZeroCnt], 0
	mov ecx, eax
	
	; 循环填充0
	clearZero:
		cmp dword[fillZeroCnt], ebx
		je clearFinished

		mov eax, ecx
		add eax, dword[fillZeroCnt]
		mov dl, 48
		mov byte[eax], dl

		inc dword[fillZeroCnt]

		jmp clearZero

	clearFinished:
		; 结束清空计数器
		mov dword[fillZeroCnt], 0
		ret

reverse:
	pusha
	mov eax, 0

reverseLoop:
	cmp eax, dword[mulDigitIndex]
	je reverseFinished

	mov ebx, mulDigitResult
	add ebx, eax
	
	mov ecx, addMulDigit
	add ecx, dword[mulDigitIndex]
	sub ecx, eax
	dec ecx

	mov dl, byte[ebx]
	mov byte[ecx], dl

	inc eax

	jmp reverseLoop
	
reverseFinished:
	popa
	ret