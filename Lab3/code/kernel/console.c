
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);

PRIVATE int  print_color;
PRIVATE int  start_find_index;

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	// 被init_tty()调用
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	// 初始化颜色
	print_color = DEFAULT_CHAR_COLOR;
	// 初始化第一个查找元素位置
	start_find_index = 0;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;
	p_tty->p_console->is_ctrl_z = 0;
	
	position = 0;
	esc_postion = 0;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		// 除以二是因为屏幕上的一个字符代表2个字节
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		// out_char(p_tty->p_console, nr_tty + '0');
		// out_char(p_tty->p_console, '#');
	}
	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	// 检查是否是当前的控制台
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   ctrlZ
 *======================================================================*/
PRIVATE void ctrlZ(CONSOLE* p_con, char c){
	p_con->chars[position] = c;
	position++;
}

/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	// 向控制台输出字符
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	if(find_pattern == 2 && ch != '\1'){
		// 查找后，不响应Esc以外的部分。
		return;
	}

	// 0是插入操作
	int ctrl_pattern = 0;
	// ctrl中存储的字符
	char next_char = '\b';

	switch(ch) {
		case '\n':
			if(!find_pattern){
				// 正常模式换行
				if (p_con->cursor < p_con->original_addr +
						p_con->v_mem_limit - SCREEN_WIDTH) {
					*p_vmem++ = '\n';
					*p_vmem++ = NO_COLOR;
					p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
						((p_con->cursor - p_con->original_addr) /
						SCREEN_WIDTH + 1);
				}
			}else{
				// 执行查找，并且变色
				if(find_pattern == 1){
					// 查找字符串长度
					int length = p_con->cursor - start_find_index;
					// 第一个char指针
					u8* current_char = (u8*)(V_MEM_BASE);
					// 外层指针
					int i = 0;
					// unsigned int & int
					while(i <= (int)(start_find_index - p_con->original_addr - length) * 2){
						// 
						if(*(current_char + i) == *(current_char + start_find_index * 2)){
							int tmp = 1;
							int is_match = 1;
							// 进行匹配
							while(tmp < length){
								if(*(current_char + i + tmp * 2) != *(current_char + (start_find_index + tmp) * 2)){
									is_match=0;
									break;
								}
								tmp++;
							}
							tmp = 0;
							// 对已经匹配的进行染色
							if(is_match){
								while(tmp < length){
									// 不修改\t
									if(*(current_char + i + tmp * 2) == '\t'){
										tmp++;
										continue;
									}
									*(current_char + i + tmp * 2 + 1) = RED_COLOR;
									tmp++;
								}
							}
						}
						i += 2;
					}
					// 进入等待结束模式
					find_pattern = 2;
				}
			}
			break;
		case '\b':
			ctrl_pattern = 1;
			if (p_con->cursor > p_con->original_addr) {
				if(p_con->cursor % SCREEN_WIDTH == 0){
					//检查换行
					int i = 2;
					while((*(p_vmem-i) != '\n' && *(p_vmem-i) == '\0')){
						p_con->cursor--;
						i += 2;
						if((i/2 - 1) % SCREEN_WIDTH == 0){
							break;
						}
					}
					// 消除换行符
					*(p_vmem-i) = ' ';
					*(p_vmem-i + 1) = print_color;
					p_con->cursor--;
					// 不必删除上一行末尾
					next_char = '\n';
					break; // 脱离case
				}
				// 检查有无tab
				if(*(p_vmem - 2*TAB_WIDTH) == '\t'){
					int i = 0;
					while(i <= 2 * TAB_WIDTH){
						*(p_vmem - i) = ' ';
						*(p_vmem - i + 1) = print_color;
						p_con->cursor--;
						i+=2;
					}
					p_con->cursor++;
					next_char = '\t';
					break;// 脱离case
				}
				// 没有空格也没有tab，正常删除
				p_con->cursor--;
				next_char = *(p_vmem-2);
				*(p_vmem-2) = ' ';
				*(p_vmem-1) = print_color;
			}
			break;
		case '\t':
			if (p_con->cursor >= p_con->original_addr) {
				// 默认使用tab
				*p_vmem++ = '\t';
				*p_vmem++ = NO_COLOR;
				p_con->cursor++;
				int i = 1;
				while(i < TAB_WIDTH){
					*p_vmem++ = ' ';
					*p_vmem++ = NO_COLOR;
					p_con->cursor ++;
					i++;
				}
			}
			break;
		case '\1':// Esc
			// Esc 进入/退出
			if(find_pattern == 0){
				// 进入Esc模式
				find_pattern = find_pattern ? 0 : 1;
				print_color = find_pattern ? RED_COLOR : DEFAULT_CHAR_COLOR;
				start_find_index = p_con->cursor;
				esc_postion = position;
			}else if(find_pattern == 2){
				// 结束Esc模式，恢复颜色
				u8* current_char = (u8*)(V_MEM_BASE);
				// 双指针
				unsigned int i = 0;

				while(i < (p_con->cursor - p_con->original_addr ) * 2){
					if( p_con->original_addr * 2 + i >= start_find_index * 2){
						*(current_char + i) = ' ';
					}
					if(*(current_char + i) == '\t'){
						if(i >= (start_find_index - p_con->original_addr) * 2){
							*(current_char + i) = ' ';
							*(current_char + i + 1) = DEFAULT_CHAR_COLOR;
						}
					}else if(*(current_char + i) != '\n'){
						*(current_char + i + 1) = DEFAULT_CHAR_COLOR;
					}
					i+=2;
				
				}
				p_con->cursor = start_find_index;
				// 解决光标问题
				p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
				*(p_vmem + 1) = DEFAULT_CHAR_COLOR;
				find_pattern = 0;
				print_color = DEFAULT_CHAR_COLOR;

				// 处理栈
				position = esc_postion;
			}
			break;
		case '\2':
			// ctrl_pattern本身操作不进入
			ctrl_pattern = -1;
			// 添加锁
			p_con->is_ctrl_z=1;
			if(position >= 1){
				// 划分开查找模式和正常模式
				int tmp = 1;
				if(find_pattern == 1){
					if(position <= esc_postion + 1){
						tmp = 0;
					}
				}
				if(tmp == 1){
					position--;
					out_char(p_con, p_con->chars[position]);
				}
			}
			// 解除锁
			p_con->is_ctrl_z=0;

			break;
		default:
			// 输出一般字符
			if (p_con->cursor <
					p_con->original_addr + p_con->v_mem_limit - 1) {
				*p_vmem++ = ch;
				*p_vmem++ = print_color;
				*(p_vmem+1) = print_color;
				p_con->cursor++;
			}
			break;
	}
	// 不是ctrl-z以及没有锁，则添加
	if(ctrl_pattern != -1 && p_con->is_ctrl_z == 0){
		position = position % CTRL_LENGTH;
		ctrlZ(p_con, next_char);
	}
	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	// 调整光标位置
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	// 切换控制台函数
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			// 向上滚屏调整
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			// 向下滚屏调整
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}


/*======================================================================*
                           clear
*======================================================================*/
PUBLIC void clear(CONSOLE* p_con)
{
	// 清空屏幕
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	int i = 2;
    while(p_con->cursor != p_con->original_addr){
		*(p_vmem-i) = '\0';
		*(p_vmem-i+1) = DEFAULT_CHAR_COLOR;
		i+=2;
		p_con->cursor--;
	}
	// 清空ctrl缓冲区
	position = 0;
	esc_postion = 0;
	flush(p_con);
}