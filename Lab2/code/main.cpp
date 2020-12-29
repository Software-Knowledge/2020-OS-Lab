#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <fstream>
using namespace std;

// ������������������������������������������������������������
// my_print

// ��
const char *COLOR_RED = "\033[31m";
// ��
const char *COLOR_RESET = "\033[0m";

extern "C" {
	void print(const char* pointer);
}
//void print(const char * strs) {
//	std::cout << strs;
//}

// ��ӡ����
void printNumber(int number) {
	print(std::to_string(number).c_str());
}

// ��ӡ�س�
void println() {
	print("\n");
}

// ��ӡ�ַ�������ӡ����
void println(const char *strs) {
	print(strs);
	print("\n");
}

// ʹ�ú�ɫ��ӡstr��Ȼ��ת���ɰ�ɫ
void printWithColor(const char * str) {
	print(COLOR_RED);
	print(str);
	print(COLOR_RESET);
}

// ���ļ�����ʽ���
void printlsl(int dirCount, int fileCount) {
	print("  ");
	printNumber(dirCount);
	print(" ");
	printNumber(fileCount);
	println("");
}

// ���ļ��е���ʽ���
void printlsl(std::string parent, int dirCount, int fileCount) {
	print(parent.c_str());
	print(" ");
	printNumber(dirCount);
	print(" ");
	printNumber(fileCount);
	println(": ");
}

// ������������������������������������������������������������
// Item File
class File;
class Item {
public:
	// ��ȡ��������
	virtual vector<File*> getSubItems() = 0;
	// ��ʾ��ǰ�Ƿ���һ��Ŀ¼
	virtual bool isDirectory() = 0;
	// С�˲���ϵͳ
	unsigned readToInt(const char *str, int offset = 0, int length = 1) {
		/*
			��ȡ��str + offset������Ϊlength������ֵ
		*/
		unsigned result = 0;
		for (int i = 0; i < length; i++, offset++) {
			// ���� 8 * i ������������֣�Ȼ��ƫ�ƶ�Ӧ���֣�������ȥ
			result += (unsigned char)(str[offset]) << i * 8;
		}
		return result;
	}
};

class File : public Item {

public:
	// ����
	string name;
	// ��չ��
	string ext;
	// ��ȡ�ǲ���������
	bool isSpecial;
	// �Ƿ����ļ���
	bool isDir;
	// ���һ��д��ʱ��
	int writeTime;
	// ���һ��д������
	int writeDate;
	// ����ָ��
	const char *contentPointer;
	// �ļ���С
	int size;
	// ��������ʼָ��
	const char* dataSectors;
	// FAT��Ӧָ��λ��
	const char* FATSector;
	// ��ʼ�غ�
	int startCluster;

	// ��ʼ����
	File(const char *start, const char *dataSectors, const char* FATSector)
		: dataSectors(dataSectors), FATSector(FATSector)
	{
		// ��ȡ�ļ���
		string name(start, 8);
		for (char i : name) {
			if (i != ' ') {
				this->name += i;
			}
		}
		// ��ȡ�ļ���չ��
		string ext(start + 8, 3);
		for (char i : ext) {
			if (i != ' ') {
				this->ext += i;
			}
		}
		// ��ȡ�Ƿ����ļ���
		isDir = readToInt(start, 0xB, 1) == 0x10;
		// ��ȡ���һ��д��ʱ��
		writeTime = readToInt(start, 0x16, 2);
		// ��ȡ���һ��д������
		writeDate = readToInt(start, 0x18, 2);
		// ��ȡ�ļ���С
		size = readToInt(start, 0x1C, 4);
		// ��ȡ������Ӧ�Ŀ�ʼ����
		startCluster = readToInt(start, 0x1A, 2);
		// ��ȡ��ǰ������ָ��
		contentPointer = dataSectors + (startCluster - 2) * 512;
		// ��ȡ�ǲ�����������籾���ļ���.���ϼ��ļ���..
		isSpecial = start[0] == '.';
	}

	// ��ȡȫ��
	string getFullName() {
		return isDir ? name: name + '.' + ext;
	}

	// ��ȡ�Ƿ����ļ���
	bool isDirectory() override {
		return isDir;
	}

	// ��ȡ���ֻ�����ļ���ʱ�Ž��в����������ļ����򷵻ؿ�
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
	// ��ȡ����
	string readContent()
	{
		string fileContent;
		unsigned clusterNo = startCluster;
		// ���غŴ��ڵ���0xFF8��ʾ�Ѿ��ﵽ���һ��
		while (clusterNo < 0xFF8) {
			if (clusterNo >= 0xFF0) {
				throw "encountered bad cluster";
			}
			// ��ǰ��������ָ��
			const char* contentPointer = dataSectors + (clusterNo - 2) * 512;
			fileContent += string(contentPointer, 512);
			// �ҵ���Ӧ����һ���غ�
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
	// ��ȡ֮���FAT���õ���һ�أ�һ���غ�1.5 �ֽ�
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


// ������������������������������������������������������������
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
	// ����дΪTrue
	bool isDirectory() override {
		return true;
	}

	// ��ȡ���е�����
	vector<File*> getSubItems() override {
		vector<File*> rootItems;
		// �õ���Ŀ¼����ָ��
		char* rootPointer = RootDirSectors();
		for (int i = 0;; i += 0x20) {
			if (rootPointer[i] != 0) {
				// �ҵ����ֿ�ʼ�ĵط���ÿһ��Ŀ¼��Ĵ�СΪ0x20�ֽڣ����Լ�0x20
				rootItems.push_back(
					new File(rootPointer + i, DataSectors(), FATSector()));
			}
			else {
				break;
			}
		}
		return rootItems;
	}
	// FAT���ȡ
	char *FATSector()
	{
		// boot����ռ�ݵ�������
		const int BPB_ResvdSecCnt = readToInt(content, 14, 2);
		return content + (BPB_ResvdSecCnt * 512); // sector 1
	}

	// ��Ŀ¼����ȡ
	char *RootDirSectors() {
		// boot����ռ�ݵ�������
		const int BPB_ResvdSecCnt = readToInt(content, 14, 2);
		// �ܹ���FAT�ı������
		const int BPB_NumFATs = readToInt(content, 16, 1);
		// һ��FAT��ռλ��С
		const int BPB_FATSz16 = readToInt(content, 22, 2);
		// 9 Sections for FAT1��9 Sections for FAT2
		return content + ((BPB_ResvdSecCnt + BPB_NumFATs * BPB_FATSz16) * 512);
	}

	// ��������ȡ
	char *DataSectors() {
		// ÿ�����ֽ�����Bytes/Sector��
		const int BPB_BytePerSec = readToInt(content, 11, 2);
		// boot����ռ�ݵ�������
		const int BPB_ResvdSecCnt = readToInt(content, 14, 2);
		// �ܹ���FAT�ı������
		const int BPB_NumFATs = readToInt(content, 16, 1);
		// ��Ŀ¼���ļ������
		const int BPB_RootEntCnt = readToInt(content, 17, 2);
		// һ��FAT��ռλ��С
		const int BPB_FATSz16 = readToInt(content, 22, 2);
		// ��Ŀ¼��������������Ŀ¼����Ŀ¼����ɣ�һ��Ŀ¼���СΪ32�ֽ�
		const int RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytePerSec - 1)) / BPB_BytePerSec;
		return content + ((BPB_ResvdSecCnt + BPB_NumFATs * BPB_FATSz16 + RootDirSectors) * 512);
	}

	~Image() {
		delete content;
	}

};

// ������
Image image("./a.img");

// ����������
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
����Separator���з�str
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
commandΪ�����һ������
�з������������
vector<string>Ϊ���ص��зֺ������
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
			// ������ֶ���ո�����
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
	���Ҷ�Ӧ��Item����Root��ʼ������dirs·���н��в��ң�now ��ʾ��ǰƥ�䵽��λ��
*/
Item* findItem(Item* root, vector<string>& dirs, int now) {
	if (root == nullptr) {
		throw "Not find this Item!";
	}
	if (now == dirs.size()) {
		return root;
	}
	else {
		// ִ�й����������,�������е�����
		for (File* item : root->getSubItems()) {
			if (item->getFullName() == dirs[now]) {
				return findItem(item, dirs, now + 1);
			}
		}
		throw "Not find this Item!";
	}
}

/*
ls �����
*/
void lsOutput(Item* dir, string parent) {
	// �������·��
	print(parent.c_str());
	println(":");
	// ������е�����
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
ls ��-lָ��
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
	// ������е�����
	auto subItems = dir->getSubItems();
	int fileCount = 0, dirCount = 0;
	for (auto item : subItems) {
		// . & .. ����¼
		if (item->isDir && !item->isSpecial) {
			dirCount++;
		}
		else if (!item->isDir) {
			fileCount++;
		}
	}
	printlsl(parent, dirCount, fileCount);

	// ѭ�������ǰĿ¼����
	for (auto item : subItems) {
		// �ļ�����ļ�������
		if (!item->isDirectory()) {
			// �ļ�����ļ�������
			lsl(parent + item->getFullName());
			println();
		}
		else {
			// �ļ�����Ҫ���ֱ����Ŀ¼��ֱ�����ļ���
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
	// �ݹ�������ļ������ļ��е����
	for (auto item : subItems) {
		if (item->isDir && !item->isSpecial) {
			lslOutput(item, parent + item->name + '/');
		}
	}
}

/*
ls -lָ��
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
cat����
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
	һ������
*/
void input() {

	string input;
	getline(cin, input);
	vector<string> commandList = commandSplit(input);
	println();

	if (commandList.size() == 0) {
		return;
	}
	// ls �����
	if (commandList[0] == "ls") {
		// û�и������
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
			// ����ֻ�ܴ�ӡһ��
			if (target >= 2)throw "Too many to ls!";
			// ����ls -l������ls address
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
	// cat ����
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
	// exit�˳�
	else if (commandList[0] == "exit"&&commandList.size() == 1) {
		throw 0;
	}
	// δ֪����
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
			// �����˳�
			return returnCode;
		}
		println();
	}
}