#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <fstream>
using namespace std;

// ——————————————————————————————
// my_print

// 红
const char *COLOR_RED = "\033[31m";
// 白
const char *COLOR_RESET = "\033[0m";

extern "C" {
	void print(const char* pointer);
}
//void print(const char * strs) {
//	std::cout << strs;
//}

// 打印数字
void printNumber(int number) {
	print(std::to_string(number).c_str());
}

// 打印回车
void println() {
	print("\n");
}

// 打印字符串，打印后换行
void println(const char *strs) {
	print(strs);
	print("\n");
}

// 使用红色打印str，然后转换成白色
void printWithColor(const char * str) {
	print(COLOR_RED);
	print(str);
	print(COLOR_RESET);
}

// 非文件夹形式输出
void printlsl(int dirCount, int fileCount) {
	print("  ");
	printNumber(dirCount);
	print(" ");
	printNumber(fileCount);
	println("");
}

// 以文件夹的形式输出
void printlsl(std::string parent, int dirCount, int fileCount) {
	print(parent.c_str());
	print(" ");
	printNumber(dirCount);
	print(" ");
	printNumber(fileCount);
	println(": ");
}

// ——————————————————————————————
// Item File
class File;
class Item {
public:
	// 获取所有子项
	virtual vector<File*> getSubItems() = 0;
	// 表示当前是否是一个目录
	virtual bool isDirectory() = 0;
	// 小端操作系统
	unsigned readToInt(const char *str, int offset = 0, int length = 1) {
		/*
			获取从str + offset，长度为length的整数值
		*/
		unsigned result = 0;
		for (int i = 0; i < length; i++, offset++) {
			// 首先 8 * i 计算出左移数字，然后偏移对应数字，最后加上去
			result += (unsigned char)(str[offset]) << i * 8;
		}
		return result;
	}
};

class File : public Item {

public:
	// 名字
	string name;
	// 扩展名
	string ext;
	// 获取是不是特殊项
	bool isSpecial;
	// 是否是文件夹
	bool isDir;
	// 最后一次写入时间
	int writeTime;
	// 最后一次写入日期
	int writeDate;
	// 内容指针
	const char *contentPointer;
	// 文件大小
	int size;
	// 数据区开始指针
	const char* dataSectors;
	// FAT对应指针位置
	const char* FATSector;
	// 开始簇号
	int startCluster;

	// 初始化表
	File(const char *start, const char *dataSectors, const char* FATSector)
		: dataSectors(dataSectors), FATSector(FATSector)
	{
		// 获取文件名
		string name(start, 8);
		for (char i : name) {
			if (i != ' ') {
				this->name += i;
			}
		}
		// 获取文件扩展名
		string ext(start + 8, 3);
		for (char i : ext) {
			if (i != ' ') {
				this->ext += i;
			}
		}
		// 获取是否是文件夹
		isDir = readToInt(start, 0xB, 1) == 0x10;
		// 获取最后一次写入时间
		writeTime = readToInt(start, 0x16, 2);
		// 获取最后一次写入日期
		writeDate = readToInt(start, 0x18, 2);
		// 获取文件大小
		size = readToInt(start, 0x1C, 4);
		// 获取此条对应的开始簇数
		startCluster = readToInt(start, 0x1A, 2);
		// 获取当前的内容指针
		contentPointer = dataSectors + (startCluster - 2) * 512;
		// 获取是不是特殊项，比如本级文件夹.、上级文件夹..
		isSpecial = start[0] == '.';
	}

	// 获取全称
	string getFullName() {
		return isDir ? name: name + '.' + ext;
	}

	// 获取是否是文件夹
	bool isDirectory() override {
		return isDir;
	}

	// 获取子项，只有是文件夹时才进行操作，不是文件夹则返回空
	vector<File*> getSubItems() override {
		vector<File*> subItems;
		if (isDir) {
			for (int i = 0;; i += 0x20) {
				if (contentPointer[i] != 0) {
					subItems.push_back(new File(
						contentPointer + i, dataSectors, FATSector));
				}
				else {
					break;
				}
			}
		}
		return subItems;
	};
	// 读取内容
	string readContent()
	{
		string fileContent;
		unsigned clusterNo = startCluster;
		// 当簇号大于等于0xFF8表示已经达到最后一簇
		while (clusterNo < 0xFF8) {
			if (clusterNo >= 0xFF0) {
				throw "encountered bad cluster";
			}
			// 当前的数据区指针
			const char* contentPointer = dataSectors + (clusterNo - 2) * 512;
			fileContent += string(contentPointer, 512);
			// 找到对应的下一个簇号
			clusterNo = nextClusterNo(clusterNo);
		}

		return fileContent.substr(0, size);
	}

	void printItem() {
		if (isDir) {
			printWithColor(getFullName().c_str());
		}
		else {
			print(getFullName().c_str());
		}
	}

private:
	// 读取之后从FAT中拿到下一簇，一个簇号1.5 字节
	unsigned nextClusterNo(unsigned current) {
		bool isOdd = current % 2 == 1;
		unsigned bytes = readToInt(FATSector, current / 2 * 3, 3); // 3 -> 2, 2 / 2 * 3 = 3

		if (isOdd) {
			return bytes >> (3 * 4); // get high 1.5 bytes
		}
		else {
			return bytes & 0x000FFF; // mask the high 1.5 bytes
		}
	}
};


// ——————————————————————————————
// Image
class Image : public Item {
public:
	char *content;
	Image(const char *file) {
		const int size = 1440 * 1024;
		this->content = new char[size];
		ifstream in(file, ios::binary);
		in.read(this->content, size);
		in.close();
	}
	// 覆盖写为True
	bool isDirectory() override {
		return true;
	}

	// 获取所有的子项
	vector<File*> getSubItems() override {
		vector<File*> rootItems;
		// 拿到根目录区的指针
		char* rootPointer = RootDirSectors();
		for (int i = 0;; i += 0x20) {
			if (rootPointer[i] != 0) {
				// 找到名字开始的地方，每一个目录项的大小为0x20字节，所以加0x20
				rootItems.push_back(
					new File(rootPointer + i, DataSectors(), FATSector()));
			}
			else {
				break;
			}
		}
		return rootItems;
	}
	// FAT表读取
	char *FATSector()
	{
		// boot分区占据的扇区数
		const int BPB_ResvdSecCnt = readToInt(content, 14, 2);
		return content + (BPB_ResvdSecCnt * 512); // sector 1
	}

	// 根目录区读取
	char *RootDirSectors() {
		// boot分区占据的扇区数
		const int BPB_ResvdSecCnt = readToInt(content, 14, 2);
		// 总共的FAT的表的数量
		const int BPB_NumFATs = readToInt(content, 16, 1);
		// 一个FAT的占位大小
		const int BPB_FATSz16 = readToInt(content, 22, 2);
		// 9 Sections for FAT1，9 Sections for FAT2
		return content + ((BPB_ResvdSecCnt + BPB_NumFATs * BPB_FATSz16) * 512);
	}

	// 数据区读取
	char *DataSectors() {
		// 每扇区字节数（Bytes/Sector）
		const int BPB_BytePerSec = readToInt(content, 11, 2);
		// boot分区占据的扇区数
		const int BPB_ResvdSecCnt = readToInt(content, 14, 2);
		// 总共的FAT的表的数量
		const int BPB_NumFATs = readToInt(content, 16, 1);
		// 根目录区文件最大数
		const int BPB_RootEntCnt = readToInt(content, 17, 2);
		// 一个FAT的占位大小
		const int BPB_FATSz16 = readToInt(content, 22, 2);
		// 根目录区扇区数量，根目录区由目录项组成，一个目录项大小为32字节
		const int RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytePerSec - 1)) / BPB_BytePerSec;
		return content + ((BPB_ResvdSecCnt + BPB_NumFATs * BPB_FATSz16 + RootDirSectors) * 512);
	}

	~Image() {
		delete content;
	}

};

// 变量区
Image image("./a.img");

// 函数声明区
vector<string> split(string str, char separator);
vector<string> commandSplit(string command);
Item* findItem(Item* root, vector<string>& dirs, int now);
void lsOutput(Item* dir, string parent);
void ls(string path);
void lslOutput(Item* dir, string parent);
void lsl(string path);
void cat(string path);
void input();

/*
按照Separator来切分str
*/
vector<string> split(string str, char separator){
	vector<string> result;
	string buf;
	for (char c : str){
		if (c == separator){
			if (buf.length() > 0){
				result.push_back(buf);
			}
			buf.clear();
		}
		else{
			buf += c;
		}
	}
	if (buf.length() > 0){
		result.push_back(buf);
	}
	return result;
}

/*
command为输入的一行命令
切分命令，返回数组
vector<string>为返回的切分后的命令
*/
vector<string> commandSplit(string command) {
	string buffer = "";
	vector<string> result;

	for (int i = 0; i < command.length(); i++) {
		if (command[i] == ' ') {
			if (buffer.length() > 0) {
				if (regex_match(buffer, regex("-(l)+")))
				{
					result.push_back("-l");
				}
				else {
					result.push_back(buffer);
				}
			}
			buffer = "";
			// 处理出现多个空格的情况
			for (i; i < command.length(); i++) {
				if (command[i] != ' ') {
					i--;
					break;
				}
			}
		}
		else buffer += command[i];
	}
	if (buffer.length() > 0) {
		if (regex_match(buffer, regex("-(l)+"))){
			result.push_back("-l");
		}
		else {
			result.push_back(buffer);
		}
	}
	return result;
}

/*
	查找对应的Item，从Root开始，根据dirs路径中进行查找，now 表示当前匹配到的位置
*/
Item* findItem(Item* root, vector<string>& dirs, int now) {
	if (root == nullptr) {
		throw "Not find this Item!";
	}
	if (now == dirs.size()) {
		return root;
	}
	else {
		// 执行广度优先搜索,对其所有的子项
		for (File* item : root->getSubItems()) {
			if (item->getFullName() == dirs[now]) {
				return findItem(item, dirs, now + 1);
			}
		}
		throw "Not find this Item!";
	}
}

/*
ls 的输出
*/
void lsOutput(Item* dir, string parent) {
	// 首先输出路径
	print(parent.c_str());
	println(":");
	// 输出所有的内容
	auto subItems = dir->getSubItems();
	for (auto item : subItems) {
		item->printItem();
		print("  ");
	}
	println();
	for (auto item : subItems) {
		if (item->isDir && !item->isSpecial) {
			lsOutput(item, parent + item->name + '/');
		}
	}
}

/*
ls 无-l指令
*/
void ls(string path){
	if (path[0] != '/') {
		path = '/' + path;
	}
	if (path[path.length() - 1] != '/') {
		path += '/';
	}
	vector<string> parents = split(path, '/');
	Item *root = findItem(&image, parents, 0);
	if (root->isDirectory()) {
		lsOutput(root, path);
	}
	else {
		throw "Specified path is a file";
	}

}

void lslOutput(Item* dir, string parent) {
	// 获得所有的子项
	auto subItems = dir->getSubItems();
	int fileCount = 0, dirCount = 0;
	for (auto item : subItems) {
		// . & .. 不记录
		if (item->isDir && !item->isSpecial) {
			dirCount++;
		}
		else if (!item->isDir) {
			fileCount++;
		}
	}
	printlsl(parent, dirCount, fileCount);

	// 循环输出当前目录内容
	for (auto item : subItems) {
		// 文件输出文件名即可
		if (!item->isDirectory()) {
			// 文件输出文件名即可
			lsl(parent + item->getFullName());
			println();
		}
		else {
			// 文件夹需要输出直接子目录和直接子文件数
			printWithColor(item->name.c_str());

			if (item->isDir && !item->isSpecial) {
				auto subSubItems = item->getSubItems();
				int subfileCount = 0, subDirCount = 0;
				for (auto item : subSubItems) {
					if (item->isDir && !item->isSpecial) {
						// print(("dir is "+item->name).c_str());
						subDirCount++;
					}
					else if (!item->isDir) {
						// print(("file is "+item->name).c_str());
						subfileCount++;
					}
				}
				printlsl(subDirCount, subfileCount);
			}
			else {
				println();
			}

		}
	}
	println();
	// 递归输出本文件夹中文件夹的情况
	for (auto item : subItems) {
		if (item->isDir && !item->isSpecial) {
			lslOutput(item, parent + item->name + '/');
		}
	}
}

/*
ls -l指令
*/
void lsl(string path) {

	vector<string> parents = split(path, '/');
	Item *root = findItem(&image, parents, 0);

	if (!root->isDirectory()) {
		print((((File*)root)->name + "." + ((File*)root)->ext).c_str());
		print("  ");
		print(to_string(((File*)root)->readContent().length()).c_str());

		return;
	}
	if (path[0] != '/') {
		path = '/' + path;
	}
	if (path[path.length() - 1] != '/') {
		path += '/';
	}

	lslOutput(root, path);

}

/*
cat命令
*/
void cat(string path) {
	vector<string> parents = split(path, '/');
	Item *root = findItem(&image, parents, 0);
	if (root->isDirectory()) {
		throw "This is a directory not a file.";
	}
	print(((File*)root)->readContent().c_str());
}

/*
	一行输入
*/
void input() {

	string input;
	getline(cin, input);
	vector<string> commandList = commandSplit(input);
	println();

	if (commandList.size() == 0) {
		return;
	}
	// ls 命令处理
	if (commandList[0] == "ls") {
		// 没有更多参数
		if (commandList.size() == 1) {
			ls("/");
		}
		else {
			int target = 0;
			string dict = "";
			int isL = 0;
			for (int i = 1; i < commandList.size(); i++) {
				if (commandList[i] == "-l")isL = 1;
				else if (regex_match(commandList[i], regex("/[A-Z0-9./]+")) || commandList[i] == "/") {
					target++;
					dict = commandList[i];
				}
				else throw "Wrong param in ls command!";
			}
			// 限制只能打印一个
			if (target >= 2)throw "Too many to ls!";
			// 不是ls -l，而是ls address
			if (isL == 0) {
				ls(commandList[1]);
			}
			else {
				if (target == 1) {
					lsl(dict);
				}
				else {
					lsl("/");
				}
			}
		}
	}
	// cat 命令
	else if (commandList[0] == "cat") {
		if (commandList.size() == 1) {
			throw "Please input a path of file!";
		}
		else if (commandList.size() > 2) {
			throw "Too many file to cat, please just input 1!";
		}
		else {
			cat(commandList[1]);
		}
	}
	// exit退出
	else if (commandList[0] == "exit"&&commandList.size() == 1) {
		throw 0;
	}
	// 未知命令
	else {
		throw "Unknown command!";
	}
}

int main(){
	while (true) {
		print("> ");
		try {
			input();
		}
		catch (const char* str) {
			print(str);
		}
		catch (int returnCode) {
			// 正常退出
			return returnCode;
		}
		println();
	}
}