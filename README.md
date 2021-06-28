# Simple B+ Tree

<hr>

## Specification

* Single Binary File에 B+-tree 형태의 자료를 저장한다.
* 각각의 노드는 하나의 블락 단위로 이루어져 있다.
* 파일의 맨 앞 12 바이트는 해더로 사용 [ Block Size, Root BID, Depth ]
* 각 블락마다 식별자로 BID로 가지고 있으며 4-Byte Integer로 나타낸다.
* BID는 1부터 시작하며 0을 Null을 의미한다.
* 블락의 물리 주소는 12 + ( ( BID-1 ) * BlockSize )
* Non-Leaf Node Entry: <Key, NextLevelBID> (8 byte)
* Leaf Node Entry: <Key, Value> (8 byte)

* Operation: command
	* creation: **bptree.exe c [bptree binary file] [block_size]**
	* Insert: **bptree.exe i [bptree binary file] [records data text file]**
	* Point Search: **bptree.exe s [bptree binary file] [input text file] [output text file]**
	* Range Search: **bptree.exe r [bptree binary file] [input text file] [output text file]**
	* Print B+-tree: **bptree.exe p [bptree binary file] [output text file]**



## Implement

* Bptree Class를 통해 Create, Insertion, Point Search, Range Search, Print 기능을 가지도록 구현

* Main

	> bptree class의 객체 생성 
	>
	> main 함수의 인자로 받은 명령어에 따라 해당하는 매핑되는 객체 매서드 호출

* Creation

	> createHeader method를 통해 진행
	>
	>  입력받은 block size 값과 root ID, depth는 초기 값으로 0을 bin file header 부분에 써준다.

* Insertion

	> insert method를 통해 진행
	>
	> bin file에 header 정보만 있다면 새로운 block를 생성해주고 만약 이미 block이 생성되어 있다면 insert_search라는 method를 통해 방문한 block id를 vector에 저장해 두면서 insert될 key 값이 들어갈 leaf node의 block id 를 찾아준다.
	>
	> insert method를 통해 block에 key 값을 넣을 때 overflow가 발생한다면 leaf node의 split 를 진행하고 non leaf node에 새로운 entry를 넣어주기 위해 nonleaf_split method를 통해 미리 vector에 저장한 부모 노드의 block id를 이용하여 non leaf node에 추가할 entry 값 을 넣어주고 만약 부모 노드에도 overflow가 발생한다면 split를 진행하고 재귀적으로 함수 를 호출하여 overflow가 발생한 block의 부모 노드에 entry가 추가되도록 했습니다. 만약 Root id가 바뀐다면 file header에 저장된 root id 값을 update 해준다.

* Point search, Range search

	> find_leaf method를 통해 찾고자 하는 값이 저장된 leaf node의 block id를 찾고 해당 block에서 key 값에 해당하는 value 또는 입력받은 범위 내에 있는 key, value 값을 text file에 써주도록 하였습니다. range search는 leaf node의 마지막 4byte에 저장된 next block id를 이용하여 해당 key가 범위 내에 있는지 확인한다.

* Print b+-tree 

	> structure는 print method를 통해 root node에 저장된 key 값을 output file에 써주고 만약 root node가 leaf node가 아니라면 root node에 저장된 next level block id를 vector에 저장하여 다음 level의 모든 node에 저장된 key 값을 순차적으로 써준다.







