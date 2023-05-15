#include <unistd.h>
#include <stdio.h>
#include "bmalloc.h" 
#include <sys/mman.h>
#include <assert.h>
#include <string.h>

#if 0
#define DPRINT(func) func
#else
#define DPRINT(func)
#endif
bm_option bm_mode = BestFit ;
bm_header bm_list_head = {0, 0, 0x0 } ;

void * sibling (void * h)
{
	bm_header_ptr page_header = bm_list_head.next;
	bm_header_ptr current = page_header;
	
	//page 위치 찾기
	
	while(1){
		if(!(((char*)current - (char*)page_header) < 4096 && 0 < ((char*)current - (char*)page_header))){
			page_header = current;
		}
		if(current == h) break;
		current = current->next;
	}
	bm_header_ptr sbl;
	bm_header_ptr bm_before = NULL;
	bm_header_ptr bm_after = page_header;
	if((((char*)current - (char*)page_header) / (1 << current->size)) % 2 == 0 ){//sibling은 current의 오른쪽에
		while(bm_after != h){
			bm_before = bm_after;
			bm_after = bm_after->next;
		}
		sbl = bm_after->next;
	}
	else{//sibling은 current의 왼쪽에
		while(bm_after != h){
			bm_before = bm_after;
			bm_after = bm_after->next;
		}
		sbl = bm_before;
	}

	return sbl;
}

int fitting (size_t s) 
{
	for(int i=4; i<=12; i++){
		if(( (1<<i) - sizeof(bm_header)) >= s) return i;
	}
	return -1;
}

/**
 * @brief
 * At what point does the header exist
 * @return int
 */
int header_location(size_t s){
	int position = -1;
	int fit = fitting(s);
	DPRINT(printf("fit: %d\n",fit););
	bm_header_ptr iter = bm_list_head.next;
	if(bm_mode == BestFit){
		int i=0;
		int min = 99;
		while(iter != NULL){
			if(iter->used == 0 && iter->size >= fit && min > iter->size){
				position = i;
				DPRINT(printf("position: %d\n",position););
				min = iter->size;
				
			}
			DPRINT(printf("i: %d\n",i););
			iter = iter->next;
			i++;
		}
	}
	else if(bm_mode == FirstFit){
		int i=0;
		while(iter != NULL){
			if(iter->used == 0 && iter->size >= fit){
				position = i;
				break;
			}
			iter = iter->next;
			i++;
		}
	}
	return position;
}

void * bm_devide(int location, size_t s){
	DPRINT(printf("bm_devide <<\n"););
	bm_header_ptr iter;
	iter = bm_list_head.next;
	for(int i = 0; i < location; i++){
		iter = iter->next;
	}//도착
	int fit = fitting(s);
	while(iter->size != fit){
		int next_size = iter->size - 1;
		bm_header new_header;
		new_header.used = 0;
		new_header.size = next_size;
		new_header.next = iter->next;
        int offset = (1 << next_size);
        bm_header_ptr temp_iter = (bm_header_ptr)((char *)iter + offset);
        *temp_iter = new_header;
		iter->next = temp_iter;
		iter->size = iter->size - 1;
	}
	iter->used = 1;

	return iter;
}

void * bmalloc (size_t s) 
{
	assert(fitting(s) != -1 && s > 0); //When s is over than 4087
	int location = header_location(s);
	// 공간 없으면 or 부족하면 새로 페이지 할당
	if(bm_list_head.next == NULL || location == -1){
		bm_header_ptr iter;
		if(bm_list_head.next == NULL){// 공간이 없을 때
			bm_list_head.next = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
			iter = bm_list_head.next;
		}else{//공간이 부족할 때(새로운 페이지)
			iter = bm_list_head.next;
			while (iter->next != NULL){
				iter = iter->next;
			}
			iter->next = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
			iter = iter->next;
		}
		// 헤더 설정
		bm_header new_header;
		new_header.used = 0;
		new_header.size = 12;
		new_header.next = NULL;
		// 새로운 페이지의 처음주소에 새로운 헤더 할당.
		*iter = new_header;
		location = header_location(s);
	}
	//이제는 공간이 있음
	//나누기 작업 및 반환
	return (bm_devide(location,s) + sizeof(bm_header)) ;
}

void bfree (void * p) 
{

	// sibling 사용하기
	// 모든공간 free면 munmap
	// cascade up
	bm_header_ptr iter = (bm_header_ptr)((char*)p - sizeof(bm_header));
	iter->used = 0;
	
	if(iter->size == 12){//모두 free된 경우
		bm_header_ptr before = &bm_list_head;
		bm_header_ptr current = bm_list_head.next;
		while(iter != current){
			before = current;
			current = current->next;
		}
		before->next = iter->next;
		munmap(iter,(1 << 12));
		
		return;
	}

	bm_header_ptr sbl = (bm_header_ptr)sibling(iter);
	if(sbl->used == 0 && sbl->size == iter->size){
		if(sbl > iter){//iter를 기준으로 합치(size up)면 됨
			iter->size++;
			iter->next = sbl->next;
			//초기화 해주기
			//그리고 기준을 bfree 재호출
			memset((void*)(char*)iter+sizeof(bm_header),0x0,((1 << (iter->size)) - sizeof(bm_header)));
			bfree((char*)iter + sizeof(bm_header));
		}
		else{//sbl을 기준으로 합치면 됨
			sbl->size++;
			sbl->next = iter->next;
			memset((void*)(char*)iter+sizeof(bm_header),0x0,((1 << (iter->size)) - sizeof(bm_header)));
			bfree((char*)sbl + sizeof(bm_header));
		}
	}
	return ;

}

void * brealloc (void * p, size_t s) 
{
	// 0 -> free  V
	// NULL -> bmalloc  V
	// else -> 재할당 데이터 복사
	// s가 클수도 작을수도,
	if(s == 0 && p == NULL) return NULL;
	if(s == 0) bfree(p);
	if(p == NULL) return bmalloc(s);
	bm_header_ptr now_header = (void*)((char *)p - sizeof(bm_header));
	char* cpy = (char*)p;
	char* new_header;
	if((1 << now_header->size) <= s){
		new_header = bmalloc(s);
		for(int i=0; i<(1 << now_header->size) - sizeof(bm_header); i++){
			new_header[i] = cpy[i];
		}
		bfree(p);
		return (void*)new_header;
	}
	else{
		new_header = bmalloc(s);
		for(int i=0; i<(1 << fitting(s)) - sizeof(bm_header); i++){
			new_header[i] = cpy[i];
		}
		bfree(p);
		return (void*)new_header;
	}

	return NULL ;
}

void bmconfig (bm_option opt) 
{
	assert(opt == BestFit || opt == FirstFit);
	bm_mode = opt;
	
	return;
}

int pagecount(){
	bm_header_ptr page_header = bm_list_head.next;
	bm_header_ptr current = page_header;
	int count = -1;
	while(1){
		if(!(((char*)current - (char*)page_header) < 4096 && 0 < ((char*)current - (char*)page_header))){
			page_header = current;
			count++;
		}
		if(current == NULL) break;
		current = current->next;
	}
	return count;
}

int usingmemory(){
	int total=0;
	bm_header_ptr current = bm_list_head.next;
	while(current != NULL){
		if(current->used == 1){
			total += (1 << current->size);
		}
		current = current->next;
	}
	return total;
}

int notusingmemory(){
	int total=0;
	bm_header_ptr current = bm_list_head.next;
	while(current != NULL){
		if(current->used == 0){
			total += (1 << current->size);
		}
		current = current->next;
	}
	return total;
}

int getpayload(char* p, int size){
	int total = 0;
	for(int i=0; i< (1 << size) - sizeof(bm_header); i++){
		if(p[i] != 0) total = i+1;
	}
	return total;
}

void 
bmprint () 
{
	//1. total given to memory 2. total given to user
	//3. available total	 
	//each block :  payload , sizeV
	bm_header_ptr itr ;
	int i ;

	printf("==================== bm_list ====================\n") ;
	printf("Total amount of all given memory : %dbytes\n", (1 << 12) * pagecount());
	printf("Total amount of memory given to the users : %dbytes\n", usingmemory());
	printf("Total amount of available memory : %dbytes\n", notusingmemory());
	for (itr = bm_list_head.next, i = 0 ; itr != 0x0 ; itr = itr->next, i++) {
		printf("%3d:%p:%1d %8d:", i, ((void *) itr) + sizeof(bm_header), (int)itr->used, (int) itr->size) ;

		int j ;
		char * s = ((char *) itr) + sizeof(bm_header) ;
		for (j = 0 ; j < (itr->size >= 8 ? 8 : itr->size) ; j++) 
			printf("%02x ", s[j]) ;
		printf("size of payload:%dbytes",getpayload(s,itr->size));
		printf("\n") ;
	}
	printf("=================================================\n") ;
}