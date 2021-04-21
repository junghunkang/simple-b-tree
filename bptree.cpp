#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include<fstream>
#include<cmath>
#include<vector>
#include<algorithm>
using namespace std;

struct Indexentry {
	int key, BID;
	Indexentry() {
		key = 0;
		BID = 0;
	}
	Indexentry(int k, int bid) {
		key = k;
		BID = bid;
	}
};

struct Dataentry {
	int key, value;
	Dataentry() {
		key = 0;
		value = 0;
	}
	Dataentry(int k, int v) {
		key = k;
		value = v;
	}
};

bool operDE(const Dataentry& d1, const Dataentry& d2) { //Data enrty struct ��������
	if (d1.key < d2.key) return true;
	else if (d1.key == d2.key&&d1.value < d2.value) return true;
	else return false;
}

bool operIE(const Indexentry& i1, const Indexentry& i2) { //Index entry ��������
	if (i1.key < i2.key) return true;
	else if (i1.key == i2.key&&i1.BID < i2.BID) return true;
	else return false;
}

class BPTree {
public:
	int blocksize, RootBID, Depth;
	int min_leaf, max_leaf;  //leaf node�� ���� #ptr ����
	int min_nonleaf, max_nonleaf; //non leaf node�� ���� #prt ����
	double B; // #ptrs in a block
	const char* filename; //���ڷ� ������ bptree�� ���ϸ�
	fstream  bptree; //bptree�� ���� read-write
	BPTree();
	~BPTree();
	BPTree(const char* fileName);
	int offset(int BID);
	void createHeader(char* fileName, int blockSize);
	void insert(int key, int value);
	int insert_search(int BID, int key, int curDepth, vector<int> &parentBID); //insert�� block�� �ڸ��� �˷��ش�.
	void nonleaf_split(int key, int lchildBID, int rchildBID, vector<int> &parentBID); //non-leaf Node�� split
	void print(char* outputfile);
	int find_leaf(int BID, int key, int curDepth); //search�ϰ��� �ϴ� key�� ����� leaf nodeã��
	void search(char* outputfile, int key); //point search
	void search(char* outputfile, int startRange, int endRange);// range search;
};

BPTree::BPTree(const char* fileName) {
	filename = fileName;
	bptree.open(fileName, ios::in | ios::out | ios::binary); //�Է�,��¿뵵�� �������Ϸ� ������ ����
	if (bptree.is_open()) {
		bptree.read(reinterpret_cast<char*>(&blocksize), sizeof(blocksize));
		bptree.read(reinterpret_cast<char*>(&RootBID), sizeof(RootBID));//head�� ����� RootBID
		bptree.read(reinterpret_cast<char*>(&Depth), sizeof(Depth));//head�� ����� bptree�� Depth
	}
	B = ((blocksize - 4) / 8) + 1; //#ptrs in a block
	max_leaf = ((int)B) - 1; //#entry in a block
	max_nonleaf = ((int)B) - 1;//#entry in a block
	min_leaf = (int)ceil((B / 2)); // õ���Լ�((B)/2)
	min_nonleaf = (int)(ceil((B + 1) / 2) - 1); // õ���Լ�(B+1/2)�� key�� �����̹Ƿ� -1

}

BPTree::~BPTree() {
	bptree.close();
}

int BPTree::offset(int BID) {
	return (BID - 1)*blocksize + 12;
}

void BPTree::createHeader(char* fileName, int blockSize) {
	char* filename = fileName;
	ofstream newbinfile;
	newbinfile.open(filename, ios::binary);
	int nullvalue = 0;
	newbinfile.write(reinterpret_cast<char*>(&blockSize), sizeof(int));
	newbinfile.write(reinterpret_cast<char*>(&nullvalue), sizeof(int));
	newbinfile.write(reinterpret_cast<char*>(&nullvalue), sizeof(int));
	newbinfile.close();
}

int BPTree::insert_search(int BID, int key, int curDepth, vector<int>& parentBID) { //�����߰��� entry�� �ڸ��� ã���ش�
	int offset = ((BID - 1)*blocksize) + 12;
	int leftptr = 0;
	int childBID = 0;
	int curkey = 0;
	int _curDepth = curDepth;
	bptree.seekg(offset, ios::beg);
	int time = 1;
	if (_curDepth != Depth) { //leaf node�� �ƴϸ�
		parentBID.push_back(BID);
		while (1) {
			bptree.read(reinterpret_cast<char*>(&leftptr), sizeof(int));//���� ������
			bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));//key��
			if (time <= max_nonleaf && key < curkey) {
				childBID = leftptr; //������ ������ 
				break;
			}
			else if (curkey == 0) { //�޾ƿ� key���� 0�̸�
				childBID = leftptr;
				break;
			}
			else if (time > max_nonleaf) {  // non-leaf node�� ��� key�� ����ũ��
				childBID = leftptr; //���� ������ ������
				break;
			}
			time++;
		}
		return insert_search(childBID, key, _curDepth + 1, parentBID); //�ڽ�node�� ����.
	}
	else return BID;
}

void BPTree::nonleaf_split(int key, int lchildBID, int rchildBID, vector<int>& parentBID) {
	if (parentBID.size() == 0) { //���� rootNode�� ���� ����� �־�� �Ѵٸ�
		bptree.seekg(0, ios::end);
		int fileSize = bptree.tellg();
		int newBID = ((fileSize - 12) / blocksize) + 1; //���� ���� block�� ID
		Indexentry empty = { 0,0 };
		bptree.seekp(0, ios::end);
		bptree.write(reinterpret_cast<const char*>(&lchildBID), sizeof(int));
		bptree.write(reinterpret_cast<const char*>(&key), sizeof(int));
		bptree.write(reinterpret_cast<const char*>(&rchildBID), sizeof(int));
		for (int i = 2; i <= max_nonleaf; i++) bptree.write(reinterpret_cast<char*>(&empty), sizeof(Indexentry));
		RootBID = newBID; //root�� ���θ�������Ƿ� root update;
		Depth++;
		//header ����
		bptree.seekp(4, ios::beg);
		bptree.write(reinterpret_cast<char*>(&RootBID), sizeof(int));
		bptree.write(reinterpret_cast<char*>(&Depth), sizeof(int));
	}
	else {
		int parent = parentBID[parentBID.size() - 1];
		int offset = ((parent - 1)*blocksize) + 12; //parent block�� offset
		parentBID.pop_back();
		vector<Indexentry> block; // ���� ����Ű�� blck�� index entry ����
		bptree.seekg(offset, ios::beg); //���������͸� parent�� block�� ������ġ�� �ű��
		int prekey, curkey, curptr;
		int cnt = 1;
		block.push_back({ key,rchildBID });
		int firstBID;
		bptree.read(reinterpret_cast<char*>(&firstBID), sizeof(int)); //non-leaf node�� ���� ������ BID���� �����Ѵ�.
		while (cnt <= max_nonleaf) {
			bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));
			bptree.read(reinterpret_cast<char*>(&curptr), sizeof(int));
			if (curptr == 0) break; //���� Ű������ ���� key���� �۴ٸ� ordered index���� -> ���� ���°�
			block.push_back({ curkey,curptr });
			cnt++;
		}
		sort(block.begin(), block.end(), operIE); //ordering�� ���� ����
		if (cnt < max_nonleaf) { //���� block�� �� �ڸ��� �ִٸ�
			bptree.seekp(offset, ios::beg); //block�� ���ۺκ����� ����
			bptree.write(reinterpret_cast<char*>(&firstBID), sizeof(int));
			for (int i = 0; i < block.size(); i++) {
				bptree.write(reinterpret_cast<char*>(&block[i]), sizeof(Indexentry)); //������ block�� entry�� ����
			}
		}
		else { //Index entry�� ��á�ٸ�
			vector<Indexentry> c;
			vector<Indexentry> c_;
			Indexentry empty;
			bptree.seekg(0, ios::end);
			int fileSize = bptree.tellg(); //file�� ũ�⸦ �����ش�, ������ block�� ���� �����ϱ� ����
			int newBID = ((fileSize - 12) / blocksize) + 1; //���� ���� block�� ID

			//non-leaf node split
			for (int i = 0; i < min_nonleaf; i++) c.push_back(block[i]);
			for (int i = min_nonleaf; i < block.size(); i++) c_.push_back(block[i]);

			//c node�� ����
			bptree.seekp(offset, ios::beg);
			bptree.write(reinterpret_cast<char*>(&firstBID), sizeof(int));
			for (int i = 0; i < c.size(); i++)
				bptree.write(reinterpret_cast<char*>(&c[i]), sizeof(Indexentry));
			for (int i = c.size(); i < max_nonleaf; i++)
				bptree.write(reinterpret_cast<char*>(&empty), sizeof(Indexentry));

			//c' node�� ����
			bptree.seekp(0, ios::end);  //�� block�� ����� ���� ������ �̵�
			bptree.write(reinterpret_cast<char*>(&c_[0].BID), sizeof(int));
			for (int i = 1; i < c_.size(); i++) bptree.write(reinterpret_cast<char*>(&c_[i]), sizeof(Indexentry));
			for (int i = c_.size(); i <= max_nonleaf; i++) bptree.write(reinterpret_cast<char*>(&empty), sizeof(Indexentry));

			nonleaf_split(c_[0].key, parent, newBID, parentBID); //�θ� split
		}
	}
}

void BPTree::insert(int key, int value) {
	if (RootBID == 0) { // bin file�� header �� �����Ҷ�
		bptree.seekp(0, ios::end);//�����͸� ������ ������.
		//key�� value�� ����
		bptree.write(reinterpret_cast<const char*>(&key), sizeof(int));
		bptree.write(reinterpret_cast<const char*>(&value), sizeof(int));

		Dataentry DE;

		int nullvalue = 0;

		for (int i = 2; i <= max_leaf; i++) { //�̹� �����Ϳ�Ʈ�� �ϳ� �����Ƿ� 2���� ���� 
											//�� �� �ִ� ��Ʈ�� ������ max_leaf-1��
			bptree.write(reinterpret_cast<char*>(&nullvalue), sizeof(int)); // ���� ��� entry 0,0���� ä���ش�
			bptree.write(reinterpret_cast<char*>(&nullvalue), sizeof(int));
		}

		bptree.write(reinterpret_cast<char*>(&nullvalue), sizeof(int)); //������ ������ 0���� ä���
		RootBID = 1;
		bptree.seekp(sizeof(int), ios::beg); //Ŀ���� ������ �տ��� sizeof(int)��ŭ �ڷ� �ű��
		bptree.write(reinterpret_cast<char*>(&RootBID), sizeof(int)); //head���� RootBID ����
		return;

	}
	// file�� ������� �ʾ�����
	vector<int> parentBID; //leafNodeŽ�������� parent�� blockID�� �����ϱ�����
	int curBID = insert_search(RootBID, key, 0, parentBID); //�� BID�� ã�´�.
	vector<Dataentry> block; // ���� ����Ű�� blck�� Dataentry �����ϱ����� ����
	bptree.seekg(offset(curBID), ios::beg); //���������͸� �ش� block�� ������ġ�� �ű��
	int prekey, curkey, curvalue;
	int cnt = 1;
	block.push_back({ key,value });
	while (cnt <= max_leaf) {
		if (cnt != 1) prekey = curkey;
		bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));
		bptree.read(reinterpret_cast<char*>(&curvalue), sizeof(int));
		if (cnt != 1 && curkey < prekey) break; //���� Ű������ ���� key���� �۴ٸ� orderindex���� -> ���� ���°�
		block.push_back({ curkey,curvalue });
		cnt++;
	}
	sort(block.begin(), block.end(), operDE); //block�� �� key�� ������ ����
	if (cnt <= max_leaf) { //���� block�� �� �ڸ��� �ִٸ�
		bptree.seekp(offset(curBID), ios::beg); //block�� ���ۺκ����� ����
		for (int i = 0; i < block.size(); i++) {
			bptree.write(reinterpret_cast<char*>(&block[i]), sizeof(Dataentry)); //������ blockd�� entry�� ����
		}
	}
	else { //Node�� ��á�ٸ� split ����
		bptree.seekg(0, ios::end);
		int fileSize = bptree.tellg(); //file�� ũ�⸦ �����ش�, ������ block�� ���� �����ϱ� ����
		int nextBID;
		bptree.seekg(offset(curBID) + blocksize - 4, ios::beg);
		bptree.read(reinterpret_cast<char*>(&nextBID), sizeof(int)); //leafNode�� ����Ǿ��ִ� nextBID����
		vector<Dataentry> c;
		vector<Dataentry> c_;
		Dataentry empty;

		//c�� c'���� ��带 ������
		for (int i = 0; i < min_leaf; i++) c.push_back(block[i]);
		for (int i = min_leaf; i < block.size(); i++) c_.push_back(block[i]);
		//Node c�� ����� dataenry ����
		bptree.seekp(offset(curBID), ios::beg); //block�� �պκ����� ����.
		for (int i = 0; i < c.size(); i++)
			bptree.write(reinterpret_cast<char*> (&c[i]), sizeof(Dataentry));
		for (int i = c.size(); i < max_leaf; i++)
			bptree.write(reinterpret_cast<char*> (&empty), sizeof(Dataentry));
		int newBID = ((fileSize - 12) / blocksize) + 1; //���� ���� block�� ID
		bptree.seekp(offset(curBID) + blocksize - 4, ios::beg); //nextBID�� �� �ڸ�, ���θ��� C'�� block��ȣ
		bptree.write(reinterpret_cast<char*> (&newBID), sizeof(int));

		//Node c_�� ����� dataentry ����
		bptree.seekp(0, ios::end); //file�� ������ ����.
		for (int i = 0; i < c_.size(); i++) {
			bptree.write(reinterpret_cast<char*> (&c_[i]), sizeof(Dataentry));
		}
		for (int i = c_.size(); i < max_leaf; i++) {
			bptree.write(reinterpret_cast<char*> (&empty), sizeof(Dataentry));
		}
		bptree.seekp(fileSize + blocksize - 4, ios::beg); //���� Node c�� ������ nextBID�� �� �ڸ�
		bptree.write(reinterpret_cast<char*> (&nextBID), sizeof(int));
		nonleaf_split(c_[0].key, curBID, newBID, parentBID);
	}

}

int BPTree::find_leaf(int BID, int key, int curDepth) { //�ش� key�� ������ ����� leafNode�� blockID search
	int offset = ((BID - 1)*blocksize) + 12;
	int leftptr = 0;
	int childBID = 0;
	int curkey = 0;
	int _curDepth = curDepth;
	bptree.seekg(offset, ios::beg);
	int time = 1;
	if (_curDepth != Depth) { //leaf node�� �ƴϸ�
		for (int i = 1; i <= max_nonleaf; i++) {
			bptree.read(reinterpret_cast<char*>(&leftptr), sizeof(int));//���� ������
			bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));//key��
			if (time <= max_nonleaf && key < curkey) {
				childBID = leftptr; //������ ������ 
				break;
			}
			else if (curkey == 0) {
				childBID = leftptr;
				break;
			}
			else if (time > max_nonleaf) {
				childBID = leftptr;
				break;
			}
			time++;
		}
		return find_leaf(childBID, key, _curDepth + 1); //�ڽ�node�� ����.
	}
	else return BID;
}

void BPTree::search(char* outputfile, int key) {
	ofstream OF;

	OF.open(outputfile, ios::app); //���� �̾��

	int bid = find_leaf(RootBID, key, 0);

	bptree.seekg(offset(bid), ios::beg);

	int curkey, value;

	for (int i = 1; i <= max_leaf; i++) {
		bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));
		bptree.read(reinterpret_cast<char*>(&value), sizeof(int));
		if (curkey == key) {
			OF << curkey << "," << value << "\n";
			break;
		}
	}

	OF.close();
}

void BPTree::search(char* outputfile, int startRange, int endRange) {
	ofstream OF;

	OF.open(outputfile, ios::app);

	int bid = find_leaf(RootBID, startRange, 0);

	int curkey, value;

	bool flag = true;
	while (1) {
		bptree.seekg(offset(bid), ios::beg);

		for (int i = 1; i <= max_leaf; i++) {
			bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));
			bptree.read(reinterpret_cast<char*>(&value), sizeof(int));
			if (curkey == 0) continue;
			if (curkey >= startRange && curkey <= endRange) {
				OF << curkey << "," << value << "\t";
			}
			else if (curkey > endRange) {
				flag = false;
				break;
			}
		}
		if (!flag) break;
		bptree.read(reinterpret_cast<char*>(&bid), sizeof(int));
		if (bid == 0) break;
	}
	OF << "\n";
	OF.close();
}

void BPTree::print(char* outputfile) {
	ofstream OF;

	OF.open(outputfile, ios::app); // ���� �̾��

	vector<vector<int>> v(3);

	v[0].push_back(RootBID);

	int id;
	int key;
	int value;

	for (int i = 0; i < 2; i++) { // level 1,2
		vector<int> k;
		if (i != Depth) {
	
			OF << "<" << i << ">" << "\n";
			for (auto blockBID : v[i]) {
				bptree.seekg(offset(blockBID), ios::beg);
				int count = 1;
				while (count <= max_nonleaf) {
					bptree.read(reinterpret_cast<char*>(&id), sizeof(int));
					bptree.read(reinterpret_cast<char*>(&key), sizeof(int));
					if (id != 0) {
						if (key != 0) k.push_back(key);
						v[i + 1].push_back(id);
					}
					count++;
				}
				bptree.read(reinterpret_cast<char*>(&id), sizeof(int));
			}
			if (id != 0) v[i + 1].push_back(id);
			OF << k[0];
			for (int x = 1; x < k.size();x++) OF << "," << k[x];
			OF << "\n";
		}
		else {
			OF << "<" << i << ">\n";
			int blockBID = v[i][0];
			while (blockBID != 0) {
				bptree.seekg(offset(blockBID), ios::beg);
				bptree.read(reinterpret_cast<char*>(&key), sizeof(int));
				bptree.read(reinterpret_cast<char*>(&value), sizeof(int));
				if (key != 0) k.push_back(key);
				for (int i = 2; i <= max_leaf; i++) {
					bptree.read(reinterpret_cast<char*>(&key), sizeof(int));
					bptree.read(reinterpret_cast<char*>(&value), sizeof(int));
					if (key != 0) k.push_back(key);
				}
				bptree.read(reinterpret_cast<char*>(&blockBID), sizeof(int));
			}
			OF << k[0];
			for (int x = 1; x < k.size(); x++) OF << "," << k[x];
			OF << "\n";
			break;
		}
	}
	OF.close();
}


int main(int argc, char* argv[]) {
	ofstream OF;
	ifstream IF;
	char* inputfile;
	char* outputfile;
	char command = argv[1][0]; // � ����� �����Ұ�����
	char* BF = argv[2]; // b+tree ����
	//Btree mybree = new Btree();

	int blocksize;
	char line[200];
	BPTree *bptree = new BPTree(BF);

	switch (command) {
	case 'c': // create index file
		blocksize = atoi(argv[3]);
		bptree->createHeader(BF, blocksize);
		break;
	case 'i': // insert reccord
		inputfile = argv[3]; //���ڷ� ������ inputfile
		IF.open(inputfile);
		if (IF.is_open()) {
			int key, value;
			while (IF.getline(line, sizeof(line))) { //���پ� �޾ƿ´�.
				char* tok;
				tok = strtok(line, ","); //line ���ۺ��� ,���ö����� ���ڿ� �ڸ���
				key = atoi(tok);
				tok = strtok(NULL, "\0"); //NULL���� ���鳪�ö����� line�ڸ���.
				value = atoi(tok);
				bptree->insert(key, value);
			}
		}
		IF.close();
		break;
	case 's': // search key in input file and print results to [output file]
		inputfile = argv[3];
		outputfile = argv[4];
		IF.open(inputfile);
		if (IF.is_open()) {
			int key;
			while (IF.getline(line, sizeof(line))) {
				int key = atoi(line);
				if (key == 0) break;
				bptree->search(outputfile, key);
			}
		}
		IF.close();
		break;
	case 'r': // search key in input file and print results to [output file]
		inputfile = argv[3];
		outputfile = argv[4];
		IF.open(inputfile);
		if (IF.is_open()) {
			int key;
			while (IF.getline(line, sizeof(line))) {
				char* tok;
				tok = strtok(line, ",");
				int startRange = atoi(tok);
				tok = strtok(NULL, "\0");
				int endRange = atoi(tok);
				bptree->search(outputfile, startRange, endRange);
			}
		}
		IF.close();
		break;
	case 'p': // print B+-Tree structure to [outout file]
		outputfile = argv[3];
		bptree->print(outputfile);
		break;
	}
	delete bptree;
	return 0;
}

