#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
using namespace std;

char* memory; //사용자가 원하는 만큼 메모리를 생성하기 위한 동적할당 변수
//섹터맵핑 테이블//
char** LBN; //lsn
char* PBN; //psn
// // // // // // 
unsigned int memory_size = 0;
unsigned int sector_size = 0;
unsigned int f_block = 0; //여분 공간의 블록 주소
int r_count = 0; //Flash_read 함수 실행횟수
int w_count = 0; //Flash_write 함수 실행횟수
int e_count = 0; //Flash_erase 함수 실행횟수
ifstream flash_r; //Flash_read() 함수를 통한 읽기 기능을 위한 파일 스트림
ofstream flash_w; //init() 함수를 통한 메모리 칩 생성을 위한 파일 스트림
fstream flash_m; //Flash_write(). Flash_erase()함수의 기능을 구현하기 위한 파일 스트림
void init();    //플래시 메모리 생성
char Flash_read(int psn); //플래시 메모리 읽기
char Flash_write(int psn, char data); //플래시 메모리 쓰기
void Flash_erase(int pbn); //플래시 메모리 지우기
void Print_table(); //매핑 테이블 출력
void FTL_read();    //FTL_read
void FTL_write();   //FTL_write
void Mapping(); //logical number와 physical number를 맵핑하는 함수
int Find_PBN(int lbn); //PSN을 찾아주는 함수

int main() {
	flash_r.open("FlashMemory.txt");
	if (flash_r.is_open()) { //이미 메모리가 생성돼있을 경우
		cout << "기존에 존재하는 플래시 메모리가 있습니다." << endl;
		cout << "init 명령어를 통해서 메모리를 다시 생성하세요!" << endl;
	}
	flash_r.close();
	string process = "";
	while (1) {
		cout << "필요하신 작업을 입력하세요." << endl;
		cout << "init = 메모리 생성\nwrite = FTL_write\nread = FTL_read\nprint = Print_table\nexit = 프로그램 종료" << endl;
		cout << "입력 : ";
		cin >> process; //사용하고 싶은 기능 입력

		if (process == "init") { //생성명령어 입력시
			init();
		}

		else if (process == "write") { //쓰기 명령어 입력시
			FTL_write();
		}

		else if (process == "read") { //읽기 명령어 입력시
			FTL_read();
		}

		else if (process == "print") { //매핑 테이블 출력 명령어 입력시
			Print_table();
		}

		else if (process == "exit") {
			if (memory_size == 0) return 0;
			delete[] memory; //동적할당 해제
			delete[] * LBN; //동적할당 해제
			delete[] PBN; //동적할당 해제
			return 0;
		}

		else {
			cout << "잘 못 입력하셨습니다." << endl;
		}

	}
	system("pause");
}

void init() { //플래시 메모리 생성
	flash_r.open("FlashMemory.txt");
	cout << "원하는 용량을 입력하시오.(단위 : MB)";
	string size = ""; //사이즈 입력
	while (1) {
		cin >> size;
		if (atoi(size.c_str()) == 0) { //숫자로 입력안했을 경우에대한 예외처리
			cout << "다시입력해주세요" << endl;
		}
		else break;
	}
	cout << endl;
	memory_size = atoi(size.c_str()) * 1024 * 1024;
	sector_size = memory_size / 512;
	LBN = new char* [sector_size / 32 - 1](); //LBN 동적할당
	PBN = new char[sector_size/32](); //PBN 동적할당
	f_block = sector_size / 32 - 2;
	Mapping(); //매핑테이블 생성
	memory = new char[memory_size](); //사용자가 입력한 사이즈만큼 플래시 메모리 동적할당
	flash_w.open("FlashMemory.txt"); //FlashMemory.txt 파일 생성
	flash_w.write(memory, memory_size); //텍스트파일에 동적할당한 만큼 출력
	flash_w.close();
	cout << memory_size << "byte 메모리 생성" << endl;
}

char Flash_read(int psn) { //플래시 메모리 읽기
	flash_r.open("FlashMemory.txt");
	unsigned int PSN = psn; //PSN을 입력받는 변수
	char tmp = ' '; //사용자가 지정한 위치에 있는 데이터 저장할 변수
	unsigned int move = 0;
	move = PSN * 512; //move변수에 섹터 사이즈와 PSN을 곱한다
	flash_r.seekg(move, ios::beg); //해당 섹터로 이동합니다.
	tmp = flash_r.get(); //tmp 변수에 파일 위치에 있는 데이터를 저장합니다.
	if (tmp == ' ' || tmp == '\0') { //저장된 내용이 없을경우
		flash_r.close();
		++r_count;
		return 'f'; //빈공간으로 반환
	}
	flash_r.close();
	++r_count;
	return tmp; //데이터
}

char Flash_write(int psn, char data) { //플래시 메모리 쓰기
	flash_m.open("FlashMemory.txt");
	flash_r.open("FlashMemory.txt");
	unsigned int PSN = psn;
	unsigned int move = 0;
	char tmp = ' ';
	move = PSN * 512;
	flash_r.seekg(move, ios::beg); //사용자가 지정한 위치로 이동
	tmp = flash_r.get();
	if (tmp != ' ' && tmp != '\0') { //빈공간이 아닐경우
		flash_r.close();
		flash_m.close();
		return 'o'; //오버라이팅 반환
	}
	flash_r.close();
	flash_m.seekg(move, ios::beg);
	flash_m.put(data);//사용자가 입력한 데이터 저장
	flash_m.close();
	if(data != ' ') ++w_count; //빈공간을 라이팅 한게 아니면 실행횟수+1
	return 'w';
}

void Flash_erase(int pbn) { //플래시 메모리 지우기
	flash_m.open("FlashMemory.txt");
	unsigned int PSN = 0;
	int PBN = 0;
	int move = PBN * 32 * 512;
	flash_m.seekg(move, ios::beg); //해당 블록의 첫 번째 섹터로 이동
	for (int i = 0; i < 32 * 512; i++) {
		flash_m << ' '; //데이터를 지웁니다
	}
	cout << endl << PBN << "블록 데이터 삭제 완료" << endl;
	flash_m.close();
	++e_count;
}

void Print_table() { //매핑 테이블 출력하는 함수
	cout << "LSN\tPSN" << endl;
	int psn = 0;
	for (int i = 0; i < sector_size; i++) {
		psn = Find_PBN(i);
		cout << i << "\t" << psn;
	}
}

void FTL_read() {
	flash_r.open("FlashMemory.txt");
	if (!flash_r.is_open()) { //생성된 메모리가 없으면 종료
		cout << "플래시 메모리를 생성해주세요." << endl;
		flash_r.close();
		return;
	}
	flash_r.close();
	int lbn = 0; //논리 블록 넘버
	int offset = 0; //오프셋값
	int lsn = 0; //lsn값
	int pbn = 0; //pbn값
	char data = ' ';
	while (1) {
		cout << 0 << " ~ " << sector_size - 32 - 1 << "사이의 숫자로 입력하세요." << endl;
		cout << "LSN을 입력해주세요 ";
		cin >> lsn; //LSN입력
		if (lsn < 0 || lsn > sector_size - 32 - 1) {
			cout << "논리 섹터 넘버를 잘 못 입력하셨습니다." << endl;
		}
		else break;
	}
	lbn = lsn / 32;
	offset = lsn % 32;
	pbn = Find_PBN(lbn);
	data = Flash_read(pbn* 32 + offset);
	if (data != 'f') {
		cout << "PBN : " << pbn << " offset : " << offset <<" data : " << data << endl;

	}
	else {
		cout << "PBN : " << pbn << " offset : " << offset << "Free Space" << endl;
	}
	r_count = 0;
}

void FTL_write() {
	int lbn = 0;
	int pbn = 0;
	int lsn = 0;
	int free = 0;
	int offset = 0;
	char data = ' ';
	char space = ' ';
	char backup_d = ' ';
	flash_r.open("FlashMemory.txt");
	if (!flash_r.is_open()) { //생성된 메모리가 없으면 종료
		cout << "플래시 메모리를 생성해주세요." << endl;
		flash_r.close();
		return;
	}
	flash_r.close();
	cout << "LSN과 입력할 데이터를 입력하세요.";
	while (1) {
		cout << 0 << " ~ " << sector_size - 32 - 1 << "사이의 숫자로 입력하세요." << endl;
		cout << "LSN과 데이터를 입력해주세요 ";
		cin >> lsn >> data; //LSN과 저장하고 싶은 데이터를 입력
		if (lsn < 0 || lsn > sector_size - 32 - 1) {
			cout << "물리 섹터 넘버를 잘 못 입력하셨습니다." << endl;
		}
		else break;
	}
	
	lbn = lsn / 32; //논리 블록 넘버로 변환
	offset = lsn % 32; //섹터 위치를 찾을 나머지값
	pbn = Find_PBN(lbn); //논리 블록에 매핑된 물리 불록을 찾는다
	space = Flash_write(pbn*32+offset, data); // 블록값 * 블록당 섹터개수 + 나머지값
	if (space == 'o') { //오버라이팅일 경우;
		free = f_block; //여분 공간 주소를 불러옴
		f_block = pbn; //여분 공간 주소 변경
		cout << "이미 사용된 공간입니다.\n업데이트를 진행합니다." << endl;
		int move = offset;
		int p_offset = 0; //첫 블록의 위치
		space = Flash_write(free * 32 + offset, data); //업데이트 되는 위치에 데이터 입력
		for (int i = 0; i < 32; i++) {
			pbn = Find_PBN(lbn);
			backup_d = Flash_read(pbn * 32 + p_offset);
			if (backup_d == 'f') backup_d = ' ';
			space = Flash_read(free * 32 + move);
			if (space == data) { //한블록 씩 이동하다가 사용자가 입력한 데이터와 만나는 경우
				move++;
				continue; //한 칸 건너 뛰기
			}
			else if (space == 'f') { //빈공간일 경우에 입력
				Flash_write(free * 32 + move, backup_d);
			}
			move++;
		}
		LBN[lbn] = &PBN[free]; //매핑 테이블 정보 변경
		Flash_erase(f_block);
		cout << "쓰기 " << w_count << "회 읽기 " << r_count << "회 지우기 " << e_count << "회 작성된 PBN : " << free << endl;
		w_count = 0; //횟수 초기화
		r_count = 0;
		e_count = 0;
	}

	else { //오버라이팅이 아닐경우
		cout << "쓰기 완료." << endl;
		cout << "쓰기 " << w_count << "회 읽기 " << r_count << "회 지우기 " << e_count << "회 PBN : " << pbn << endl;
		w_count = 0; //횟수 초기화
		r_count = 0;
		e_count = 0;
	}
}

void Mapping() {
	for (int i = 0; i < sector_size/512 - 1; i++) {
		LBN[i] = &PBN[i];
	}
}

int Find_PBN(int lbn) { //PSN을 찾는 함수
	int pbn = 0;
	for (int i = 0; i < sector_size; i++) {
		if (LBN[lbn] == &PBN[i]) { //맵핑된 주소값이 같은경우
			pbn = i; //PSN
			break;
		}
	}
	return pbn; //PBN을 리턴합니다.
}