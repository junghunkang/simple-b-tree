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

bool operDE(const Dataentry& d1, const Dataentry& d2) { //Data enrty struct 정렬조건
	if (d1.key < d2.key) return true;
	else if (d1.key == d2.key&&d1.value < d2.value) return true;
	else return false;
}

bool operIE(const Indexentry& i1, const Indexentry& i2) { //Index entry 정렬조건
	if (i1.key < i2.key) return true;
	else if (i1.key == i2.key&&i1.BID < i2.BID) return true;
	else return false;
}

class BPTree {
public:
	int blocksize, RootBID, Depth;
	int min_leaf, max_leaf;  //leaf node에 들어가는 #ptr 범위
	int min_nonleaf, max_nonleaf; //non leaf node에 들어가는 #prt 범위
	double B; // #ptrs in a block
	const char* filename; //인자로 들어오는 bptree의 파일명
	fstream  bptree; //bptree의 파일 read-write
	BPTree();
	~BPTree();
	BPTree(const char* fileName);
	int offset(int BID);
	void createHeader(char* fileName, int blockSize);
	void insert(int key, int value);
	int insert_search(int BID, int key, int curDepth, vector<int> &parentBID); //insert할 block의 자리를 알려준다.
	void nonleaf_split(int key, int lchildBID, int rchildBID, vector<int> &parentBID); //non-leaf Node의 split
	void print(char* outputfile);
	int find_leaf(int BID, int key, int curDepth); //search하고자 하는 key가 저장된 leaf node찾기
	void search(char* outputfile, int key); //point search
	void search(char* outputfile, int startRange, int endRange);// range search;
};

BPTree::BPTree(const char* fileName) {
	filename = fileName;
	bptree.open(fileName, ios::in | ios::out | ios::binary); //입력,출력용도로 이진파일로 파일을 연다
	if (bptree.is_open()) {
		bptree.read(reinterpret_cast<char*>(&blocksize), sizeof(blocksize));
		bptree.read(reinterpret_cast<char*>(&RootBID), sizeof(RootBID));//head에 저장된 RootBID
		bptree.read(reinterpret_cast<char*>(&Depth), sizeof(Depth));//head에 저장된 bptree의 Depth
	}
	B = ((blocksize - 4) / 8) + 1; //#ptrs in a block
	max_leaf = ((int)B) - 1; //#entry in a block
	max_nonleaf = ((int)B) - 1;//#entry in a block
	min_leaf = (int)ceil((B / 2)); // 천장함수((B)/2)
	min_nonleaf = (int)(ceil((B + 1) / 2) - 1); // 천장함수(B+1/2)에 key의 개수이므로 -1

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

int BPTree::insert_search(int BID, int key, int curDepth, vector<int>& parentBID) { //새로추가될 entry의 자리를 찾아준다
	int offset = ((BID - 1)*blocksize) + 12;
	int leftptr = 0;
	int childBID = 0;
	int curkey = 0;
	int _curDepth = curDepth;
	bptree.seekg(offset, ios::beg);
	int time = 1;
	if (_curDepth != Depth) { //leaf node가 아니면
		parentBID.push_back(BID);
		while (1) {
			bptree.read(reinterpret_cast<char*>(&leftptr), sizeof(int));//왼쪽 포인터
			bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));//key값
			if (time <= max_nonleaf && key < curkey) {
				childBID = leftptr; //범위에 맞으면 
				break;
			}
			else if (curkey == 0) { //받아온 key값이 0이면
				childBID = leftptr;
				break;
			}
			else if (time > max_nonleaf) {  // non-leaf node의 모든 key값 보다크면
				childBID = leftptr; //제일 오른쪽 포인터
				break;
			}
			time++;
		}
		return insert_search(childBID, key, _curDepth + 1, parentBID); //자식node로 간다.
	}
	else return BID;
}

void BPTree::nonleaf_split(int key, int lchildBID, int rchildBID, vector<int>& parentBID) {
	if (parentBID.size() == 0) { //만약 rootNode를 새로 만들어 주어야 한다면
		bptree.seekg(0, ios::end);
		int fileSize = bptree.tellg();
		int newBID = ((fileSize - 12) / blocksize) + 1; //새로 만들 block의 ID
		Indexentry empty = { 0,0 };
		bptree.seekp(0, ios::end);
		bptree.write(reinterpret_cast<const char*>(&lchildBID), sizeof(int));
		bptree.write(reinterpret_cast<const char*>(&key), sizeof(int));
		bptree.write(reinterpret_cast<const char*>(&rchildBID), sizeof(int));
		for (int i = 2; i <= max_nonleaf; i++) bptree.write(reinterpret_cast<char*>(&empty), sizeof(Indexentry));
		RootBID = newBID; //root를 새로만들었으므로 root update;
		Depth++;
		//header 갱신
		bptree.seekp(4, ios::beg);
		bptree.write(reinterpret_cast<char*>(&RootBID), sizeof(int));
		bptree.write(reinterpret_cast<char*>(&Depth), sizeof(int));
	}
	else {
		int parent = parentBID[parentBID.size() - 1];
		int offset = ((parent - 1)*blocksize) + 12; //parent block의 offset
		parentBID.pop_back();
		vector<Indexentry> block; // 현재 가리키는 blck에 index entry 저장
		bptree.seekg(offset, ios::beg); //파일포인터를 parent의 block의 시작위치로 옮긴다
		int prekey, curkey, curptr;
		int cnt = 1;
		block.push_back({ key,rchildBID });
		int firstBID;
		bptree.read(reinterpret_cast<char*>(&firstBID), sizeof(int)); //non-leaf node의 가장 왼쪽의 BID값을 저장한다.
		while (cnt <= max_nonleaf) {
			bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));
			bptree.read(reinterpret_cast<char*>(&curptr), sizeof(int));
			if (curptr == 0) break; //이전 키값보다 현재 key값이 작다면 ordered index위배 -> 값이 없는것
			block.push_back({ curkey,curptr });
			cnt++;
		}
		sort(block.begin(), block.end(), operIE); //ordering을 위해 정렬
		if (cnt < max_nonleaf) { //만약 block에 들어갈 자리가 있다면
			bptree.seekp(offset, ios::beg); //block의 시작부분으로 간다
			bptree.write(reinterpret_cast<char*>(&firstBID), sizeof(int));
			for (int i = 0; i < block.size(); i++) {
				bptree.write(reinterpret_cast<char*>(&block[i]), sizeof(Indexentry)); //저장한 block의 entry를 쓴다
			}
		}
		else { //Index entry가 꽉찼다면
			vector<Indexentry> c;
			vector<Indexentry> c_;
			Indexentry empty;
			bptree.seekg(0, ios::end);
			int fileSize = bptree.tellg(); //file의 크기를 구해준다, 마지막 block을 새로 생성하기 위함
			int newBID = ((fileSize - 12) / blocksize) + 1; //새로 만들 block의 ID

			//non-leaf node split
			for (int i = 0; i < min_nonleaf; i++) c.push_back(block[i]);
			for (int i = min_nonleaf; i < block.size(); i++) c_.push_back(block[i]);

			//c node에 저장
			bptree.seekp(offset, ios::beg);
			bptree.write(reinterpret_cast<char*>(&firstBID), sizeof(int));
			for (int i = 0; i < c.size(); i++)
				bptree.write(reinterpret_cast<char*>(&c[i]), sizeof(Indexentry));
			for (int i = c.size(); i < max_nonleaf; i++)
				bptree.write(reinterpret_cast<char*>(&empty), sizeof(Indexentry));

			//c' node에 저장
			bptree.seekp(0, ios::end);  //새 block을 만들기 위해 끝으로 이동
			bptree.write(reinterpret_cast<char*>(&c_[0].BID), sizeof(int));
			for (int i = 1; i < c_.size(); i++) bptree.write(reinterpret_cast<char*>(&c_[i]), sizeof(Indexentry));
			for (int i = c_.size(); i <= max_nonleaf; i++) bptree.write(reinterpret_cast<char*>(&empty), sizeof(Indexentry));

			nonleaf_split(c_[0].key, parent, newBID, parentBID); //부모 split
		}
	}
}

void BPTree::insert(int key, int value) {
	if (RootBID == 0) { // bin file에 header 만 존재할때
		bptree.seekp(0, ios::end);//포인터를 끝으로 보낸다.
		//key와 value를 쓴다
		bptree.write(reinterpret_cast<const char*>(&key), sizeof(int));
		bptree.write(reinterpret_cast<const char*>(&value), sizeof(int));

		Dataentry DE;

		int nullvalue = 0;

		for (int i = 2; i <= max_leaf; i++) { //이미 데이터엔트리 하나 들어갔으므로 2부터 시작 
											//들어갈 수 있는 엔트리 개수는 max_leaf-1개
			bptree.write(reinterpret_cast<char*>(&nullvalue), sizeof(int)); // 남은 모든 entry 0,0으로 채워준다
			bptree.write(reinterpret_cast<char*>(&nullvalue), sizeof(int));
		}

		bptree.write(reinterpret_cast<char*>(&nullvalue), sizeof(int)); //마지막 포인터 0으로 채운다
		RootBID = 1;
		bptree.seekp(sizeof(int), ios::beg); //커서를 파일의 앞에서 sizeof(int)만큼 뒤로 옮긴다
		bptree.write(reinterpret_cast<char*>(&RootBID), sizeof(int)); //head에서 RootBID 갱신
		return;

	}
	// file이 비어있지 않았을때
	vector<int> parentBID; //leafNode탐색과정중 parent의 blockID를 저장하기위해
	int curBID = insert_search(RootBID, key, 0, parentBID); //들어갈 BID를 찾는다.
	vector<Dataentry> block; // 현재 가리키는 blck에 Dataentry 저장하기위한 벡터
	bptree.seekg(offset(curBID), ios::beg); //파일포인터를 해당 block의 시작위치로 옮긴다
	int prekey, curkey, curvalue;
	int cnt = 1;
	block.push_back({ key,value });
	while (cnt <= max_leaf) {
		if (cnt != 1) prekey = curkey;
		bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));
		bptree.read(reinterpret_cast<char*>(&curvalue), sizeof(int));
		if (cnt != 1 && curkey < prekey) break; //이전 키값보다 현재 key값이 작다면 orderindex위배 -> 값이 없는것
		block.push_back({ curkey,curvalue });
		cnt++;
	}
	sort(block.begin(), block.end(), operDE); //block에 들어갈 key값 순으로 정렬
	if (cnt <= max_leaf) { //만약 block에 들어갈 자리가 있다면
		bptree.seekp(offset(curBID), ios::beg); //block의 시작부분으로 간다
		for (int i = 0; i < block.size(); i++) {
			bptree.write(reinterpret_cast<char*>(&block[i]), sizeof(Dataentry)); //저장한 blockd의 entry를 쓴다
		}
	}
	else { //Node가 꽉찼다면 split 진행
		bptree.seekg(0, ios::end);
		int fileSize = bptree.tellg(); //file의 크기를 구해준다, 마지막 block을 새로 생성하기 위함
		int nextBID;
		bptree.seekg(offset(curBID) + blocksize - 4, ios::beg);
		bptree.read(reinterpret_cast<char*>(&nextBID), sizeof(int)); //leafNode에 저장되어있던 nextBID저장
		vector<Dataentry> c;
		vector<Dataentry> c_;
		Dataentry empty;

		//c와 c'으로 노드를 나눈다
		for (int i = 0; i < min_leaf; i++) c.push_back(block[i]);
		for (int i = min_leaf; i < block.size(); i++) c_.push_back(block[i]);
		//Node c에 저장된 dataenry 쓰기
		bptree.seekp(offset(curBID), ios::beg); //block의 앞부분으로 간다.
		for (int i = 0; i < c.size(); i++)
			bptree.write(reinterpret_cast<char*> (&c[i]), sizeof(Dataentry));
		for (int i = c.size(); i < max_leaf; i++)
			bptree.write(reinterpret_cast<char*> (&empty), sizeof(Dataentry));
		int newBID = ((fileSize - 12) / blocksize) + 1; //새로 만들 block의 ID
		bptree.seekp(offset(curBID) + blocksize - 4, ios::beg); //nextBID가 들어갈 자리, 새로만들 C'의 block번호
		bptree.write(reinterpret_cast<char*> (&newBID), sizeof(int));

		//Node c_에 저장된 dataentry 쓰기
		bptree.seekp(0, ios::end); //file의 끝으로 간다.
		for (int i = 0; i < c_.size(); i++) {
			bptree.write(reinterpret_cast<char*> (&c_[i]), sizeof(Dataentry));
		}
		for (int i = c_.size(); i < max_leaf; i++) {
			bptree.write(reinterpret_cast<char*> (&empty), sizeof(Dataentry));
		}
		bptree.seekp(fileSize + blocksize - 4, ios::beg); //원래 Node c가 저장한 nextBID가 들어갈 자리
		bptree.write(reinterpret_cast<char*> (&nextBID), sizeof(int));
		nonleaf_split(c_[0].key, curBID, newBID, parentBID);
	}

}

int BPTree::find_leaf(int BID, int key, int curDepth) { //해당 key가 실제로 저장된 leafNode의 blockID search
	int offset = ((BID - 1)*blocksize) + 12;
	int leftptr = 0;
	int childBID = 0;
	int curkey = 0;
	int _curDepth = curDepth;
	bptree.seekg(offset, ios::beg);
	int time = 1;
	if (_curDepth != Depth) { //leaf node가 아니면
		for (int i = 1; i <= max_nonleaf; i++) {
			bptree.read(reinterpret_cast<char*>(&leftptr), sizeof(int));//왼쪽 포인터
			bptree.read(reinterpret_cast<char*>(&curkey), sizeof(int));//key값
			if (time <= max_nonleaf && key < curkey) {
				childBID = leftptr; //범위에 맞으면 
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
		return find_leaf(childBID, key, _curDepth + 1); //자식node로 간다.
	}
	else return BID;
}

void BPTree::search(char* outputfile, int key) {
	ofstream OF;

	OF.open(outputfile, ios::app); //파일 이어쓰기

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

	OF.open(outputfile, ios::app); // 파일 이어쓰기

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
	char command = argv[1][0]; // 어떤 명령을 실행할것인지
	char* BF = argv[2]; // b+tree 저장
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
		inputfile = argv[3]; //인자로 들어오는 inputfile
		IF.open(inputfile);
		if (IF.is_open()) {
			int key, value;
			while (IF.getline(line, sizeof(line))) { //한줄씩 받아온다.
				char* tok;
				tok = strtok(line, ","); //line 시작부터 ,나올때까지 문자열 자른다
				key = atoi(tok);
				tok = strtok(NULL, "\0"); //NULL부터 공백나올때까지 line자른다.
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

