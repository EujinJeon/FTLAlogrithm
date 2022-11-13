#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
using namespace std;

char* memory; //사용자가 원하는 만큼 메모리를 생성하기 위한 동적할당 변수
//섹터맵핑 테이블//
char** LSN; //lsn
char* PSN; //psn
// // // // // // 
unsigned int memory_size = 0; //메모리 사이즈 변수
unsigned int sector_size = 0; //섹터 개수 변수
unsigned int f_space = 0; //여분 공간의 주소
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
int Find_PSN(int lsn); //PSN을 찾아주는 함수

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
			delete[] * LSN; //동적할당 해제
			delete[] PSN; //동적할당 해제
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
	char decide = ' ';
	cout << "원하는 용량을 입력하시오.(단위 : MB)";
	string size = ""; //사이즈 입력
	while (1) {
		cin >> size;
		if (atoi(size.c_str())==0) { //숫자로 입력안했을 경우에대한 예외처리
			cout << "다시입력해주세요" << endl;
		}
		else break;
	}
	cout << endl;
	memory_size = atoi(size.c_str()) * 1024 * 1024;
	sector_size = memory_size / 512;
	LSN = new char*[sector_size - 96](); //LSN 동적할당
	PSN = new char [sector_size](); //PSN 동적할당
	f_space = sector_size - 96;
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
	return tmp; //데이터 리턴
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
	if (tmp != ' ' && tmp != '\0') { //해당 섹터에 데이터가 존재하는경우
		flash_m.close();
		flash_r.close();
		return 'o'; //오버라이팅이 발생했다고 알려주는 'o'반환
	}
	flash_r.close();
	flash_m.seekg(move, ios::beg);
	flash_m.put(data);//사용자가 입력한 데이터 저장
	flash_m.close();
	if(data != ' ') ++w_count;
	return 'w'; //작성완료 반환
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

void Print_table() {
	cout << "LSN\tPSN" << endl;
	int psn =0;
	for (int i = 0; i < sector_size; i++) {
		psn = Find_PSN(i);
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
	int lsn = 0;
	int psn = 0;
	char data = ' ';
	while (1) {
		cout << 0 << " ~ " << sector_size - 96 - 1 << "사이의 숫자로 입력하세요." << endl;
		cout << "LSN을 입력해주세요 ";
		cin >> lsn; //LSN입력
		if (lsn < 0 || lsn > sector_size - 96 - 1) {
			cout << "물리 섹터 넘버를 잘 못 입력하셨습니다." << endl;
		}
		else break;
	}
	psn = Find_PSN(lsn); //사용자가 입력한 LSN과 매핑된 PSN을 찾는다 
	data = Flash_read(psn); //Flash_read를 통해서 메모리에 저장된 데이터를 읽어온다
	if (data != 'f') {
		cout << "PSN : " << psn << " data : " << data << endl;
		
	}
	else {
		cout << "PSN : " << psn << "Free Space" << endl;
	}
	r_count = 0;
}

void FTL_write() {
	int lsn = 0;
	int psn = 0;
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
		cout << 0 << " ~ " << sector_size - 96 - 1 << "사이의 숫자로 입력하세요." << endl;
		cout << "LSN과 데이터를 입력해주세요 ";
		cin >> lsn >> data; //LSN과 저장하고 싶은 데이터를 입력
		if (lsn < 0 || lsn > sector_size - 96 - 1) { //LSN 범위를 벗어났을 경우
			cout << "물리 섹터 넘버를 잘 못 입력하셨습니다." << endl;
		}
		else break;
	}
	psn = Find_PSN(lsn);
	space = Flash_write(psn, data);
	int tmp = psn;
	if (space == 'o') { //프리한 공간이 아니면;
		int count = f_space; //메모리의 여분 공간주소 가져옴
		cout << "이미 사용된 공간입니다.\n업데이트를 진행합니다." << endl;
		while (1) {
			space = Flash_read(count); //해당 버퍼공간에 데이터를 불러옴
			if (space == 'f') { //빈공간일 경우
				space = Flash_write(count, data); //데이터를 입력합니다.
				LSN[lsn] = &PSN[count]; //매핑 테이블 정보를 변경합니다.
				break;
			}
			++count;
		}
		cout << "쓰기 " << w_count << "회 읽기 " << r_count << "회 지우기 " << e_count << "회 작성된 PSN : " << count << endl;
		w_count = 0; //횟수 초기화
		r_count = 0;
		e_count = 0;
	}

	else {
		cout << "쓰기 완료." << endl;
		cout << "쓰기 " << w_count << "회 읽기 " << r_count << "회 지우기 " << e_count << "회 PSN : " << psn << endl;
		w_count = 0; //횟수 초기화
		r_count = 0;
		e_count = 0;
	}
}

void Mapping() {
	for (int i = 0; i < sector_size - 96; i++) {
		LSN[i] = &PSN[i];
	}
}

int Find_PSN(int lsn) { //PSN을 찾는 함수
	int psn = 0;
	for (int i = 0; i < sector_size - 96; i++) {
		if (LSN[lsn] == &PSN[i]) { //맵핑된 주소값이 같은경우
			psn = i; //PSN
			break;
		}
	}
	return psn; //PSN을 리턴합니다.
}