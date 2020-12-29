
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
; ��ȡ���������ַ�
; output:
; num1 ��һ�����ֵ��׵�ַ
; index1 ��һ�����ֵĳ���
; flag1 ��һ�����ţ�0Ϊ��
; num2 �ڶ������ֵ��׵�ַ
; index2 �ڶ������ֵĳ���
; flag2 �ڶ������ţ�0Ϊ��
; 
getNumbers:
	pusha
	mov dword[index1], 0
	mov byte[flag1], 0

	; һλһλ��ȡ
	getNum1:
		; OPCODE 3
		mov eax, 3
		; stdin
		mov ebx, 0
		; ��ȡ��������õ�digit
		mov ecx, digit
		; ��ȡ1λ����
		mov edx, 1
		int 80h
		
		; �ո�Ϊ��ֹ��
		cmp byte[digit], " "
		je initNum2

		; ����ȡ�����ַ��õ�ָ����λ��
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

		; �س���Ϊ��ֹ��
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
; ��ɳ˷�
; input:
; num2 �ڶ������ֵĵ�ַ
; index2 �ڶ����������ֵĳ���
; 
; process:
; movCnt ��λ������
; mulCnt ��λ�˷�������
; addMulDigit ��һλ�Ľ���ĵ������
; nowPlace ��ǰλ��
; addResult �ӷ����
; 
; output:
; nowLength �������
; mulResult �˷����
; 
multiply:
	pusha
	mov eax, dword[index2]
	mov dword[mulCnt], eax

	mov dword[nowPlace], 0
	mov dword[movCnt], 0

	mov dword[mulResult], 0
	mov dword[nowLength], 0

	; �������λ���ϵ�ֵ
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

	; ִ��һλ�˷�
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
		; ִ�мӷ�
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
	
	; �ƶ�addResult��ֵ��mulResult��ȥ
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

	; ת�ƽ���
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
; һ���� ���� һ������
; input:
; nowPlace ��Ҫ����󲹳��0�ĸ���
; index1 ��һ���������ֵĳ���
; 
; process:
; mulOneCnt ��һ����������ֵĳ���
; mulDigitCarry �������е�ÿһλ�Ľ�λ������
; zeroCnt ��Ҫ����󲹳��0�ĸ���
; 
; output:
; mulDigitResult һ���� ���� һ�����ֵĽ��
; mulDigitIndex ��������λ��
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

	; ����λ
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
	
	; ��λ������
	multiplyOneDigitLoop:
		cmp dword[mulOneCnt], 0
		je multiplyOneDigitFinished

		mov ebx, num1
		add ebx, dword[mulOneCnt]
		dec ebx

		; ��num1��Ӧλ��ֵ�ŵ�eax(al)��
		mov eax, 0
		mov al, byte[ebx]
		sub eax, 48
		mul byte[mulOneDigit]

		; �˷���ɺ���ӽ�λ
		mov ecx, eax
		add ecx, dword[mulDigitCarry]
		mov dword[mulDigitCarry], 0

	; ��ȡ��λ��ֵ
	mulOverflow:
		; ����λ
		cmp ecx, 10
		jb overflowFinished

		sub ecx, 10
		inc dword[mulDigitCarry]

		jmp mulOverflow

	; ��ɽ�λ
	overflowFinished:	
		mov eax, mulDigitResult
		add eax, dword[mulDigitIndex]
		add cl, 48
		mov byte[eax], cl

		inc dword[mulDigitIndex]
		dec dword[mulOneCnt]

		jmp multiplyOneDigitLoop

multiplyOneDigitFinished:
	;����λ��λ
	cmp dword[mulDigitCarry], 0
	je multiplyOneDigitReslt

	mov eax, mulDigitResult
	add eax, dword[mulDigitIndex]
	mov cl, byte[mulDigitCarry]
	add cl, 48
	mov byte[eax], cl

	inc dword[mulDigitIndex]

multiplyOneDigitReslt:
	; ��ս�λֵ
	mov dword[mulDigitCarry], 0
	ret

; ---------------------------------
; �ӷ� 
; Input:
; num1Place ��һ���������׵�ַ
; add1Index ��һ�������ĳ���
; num2Place �ڶ����������׵�ַ
; add2Index �ڶ��������ĳ���
;
; Process:
;  ebx ��λ���
;  cnt1 ��һ������������
;  cnt2 �ڶ�������������
;
; Output:
; add3Index �ӷ�����ĳ���
; 
addition:
	; ��ս�λ
	mov ebx, 0

	; ʹ��cnt1��cnt2�洢��һ���ڶ�����������
	mov eax, dword[add1Index]
	dec eax
	mov dword[cnt1], eax

	mov eax, dword[add2Index]
	dec eax
	mov dword[cnt2], eax

	; ��ս������
	mov dword[add3Index], 0

	; ѭ����λ�ӷ�
	additionloop:
		; cnt1 == -1
		cmp dword[cnt1], -1
		je cnt2Longer

		; cnt2 == -1
		cmp dword[cnt2], -1
		je cnt1Longer

		; dh ��ȡ��һ����������
		mov eax, dword[num1Place]
		add eax, dword[cnt1]
		mov dh, byte[eax]
		sub dh, 48

		; dl ��ȡ�ڶ�����������
		mov eax, dword[num2Place]
		add eax, dword[cnt2]
		mov dl, byte[eax]
		sub dl, 48

		; �����һ���н�λ����Ҫ+1�������ÿ�ebx
		cmp ebx, 0
		je compare

		inc dh
		mov ebx, 0

		compare:
			; ������λ�ļӷ�
			add dh, dl

			; ����Ƿ������λ
			cmp dh, 10
			jl nothigher
			; ���������λ
			mov ebx, 1
			sub dh, 10

		nothigher:
			; û�в�����λ
		
			; ��Ϊ��������
			add dh, 48

			; ��������뵽addResult��Ӧ��λ����ȥ
			mov eax, addResult
			add eax, dword[add3Index]
			; eax ���������λ��
			mov byte[eax], dh
			inc dword[add3Index]

			; ���������������ĳ���
			dec dword[cnt1]
			dec dword[cnt2]
			jmp additionloop

	; ��һ�����Ƚϳ�
	cnt1Longer:
		; �жϵ�һ�����Ƿ���ȫ�����������ȥ
		cmp dword[cnt1], -1
		je additionFinished 

		; dl ��һ�����Ķ�Ӧλ�õ�����
		mov eax, dword[num1Place]
		add eax, dword[cnt1]
		mov dl, byte[eax]
	
		; �����һλ�Ƿ������
		cmp ebx, 0
		je cnt1LongerNotFlow
		
		inc dl
		mov ebx, 0

		; ע���48 + 10�Ƚ�
		cmp dl, 58
		jl cnt1LongerNotFlow

		sub dl, 10
		mov ebx, 1

	cnt1LongerNotFlow:
		; ����ǰdl�е����ַŵ���Ӧλ����
		mov eax, addResult 
		add eax, dword[add3Index] 
		mov byte[eax], dl

		inc dword[add3Index]
		dec dword[cnt1]
		jmp cnt1Longer

	; �ڶ������ֱȽϳ�
	cnt2Longer:
		; �жϵ�һ�����Ƿ���ȫ�����������ȥ
		cmp dword[cnt2], -1
		je additionFinished

		; dl �ڶ������Ķ�Ӧλ�õ�����
		mov eax, dword[num2Place]
		add eax, dword[cnt2]
		mov dl, byte[eax]

		; �����һλ�Ƿ������
		cmp ebx, 0
		je cnt2LongerNotFlow

		inc dl
		mov ebx, 0

		; ע���48 + 10�Ƚ�
		cmp dl, 58
		jl cnt2LongerNotFlow

		sub dl, 10
		mov ebx, 1

	cnt2LongerNotFlow:
		; ����ǰdl�е����ַŵ���Ӧλ����
		mov eax, addResult
		add eax, dword[add3Index]
		mov byte[eax], dl

		inc dword[add3Index]
		dec dword[cnt2]
		jmp cnt2Longer

additionFinished:
	; ������һλ�Ƿ�����˽�λ
	cmp ebx, 0
	je notOverflow

	; �����˽�λ�����Ͻ�λһ��
	mov eax, addResult 
	add eax, dword[add3Index] 
	mov byte[eax], 31h
	inc dword[add3Index]

	notOverflow:
		; û�н�λ������˳�
		mov dword[cnt1], 0
		mov dword[cnt2], 0
		mov dword[num1Place], 0
		mov dword[num2Place], 0
		ret

; --------------------------------------
; �Ӹ�λ�������
; input:
; eax ��������ֵ��׵�ַ
; printsize Ҫ��ӡ�����ĳ���
; 
; process:
; isZero ��ǰ��ӡ���ǲ���0
; print ��������ӡ������
; printCnt ������
;
; output:
; ���������̨
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

	; ��ӡһ�� 0
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
; �ӵ�λ�������
; input:
; eax ��������ֵ��׵�ַ
; printsize Ҫ��ӡ�����ĳ���
; 
; process:
; isZero �Ƿ�Ϊ0
; print ��������ӡ������
; printCnt ������
;
; output:
; ���������̨
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

	; ��ӡһ�� 0

strNotZeroResult:
	pop eax
	; ������з�
	mov eax, 4
	mov ebx, 1
	mov ecx, line
	mov edx, lineLength
	int 80h

	mov byte[isZero], 0

	ret

; ---------------------------------
; ��eax��Ӧ��ַ��ʼ��ebx���ȵ�λ��ȫ�����0
; input:
; eax ����׵�ַ
; ebx ������ݳ���
; 
; process:
; fillZeroCnt: ������
fillZero:
	; ��ʼ��������
	; ���׵�ַ��ŵ�ecx��ȥ
	mov dword[fillZeroCnt], 0
	mov ecx, eax
	
	; ѭ�����0
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
		; ������ռ�����
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