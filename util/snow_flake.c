#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

unsigned int Id_increaseNum = 0; //自增数
unsigned int Id_random = 0; //初始化的随机数
pthread_mutex_t Id_increaseNumLock = PTHREAD_MUTEX_INITIALIZER; // 为访问counter而设立的互斥
char HEX_CHARS[] = "0123456789abcdef";

char* id_generate_string()
{
    unsigned char bytes[] = "000000000000";
    
    //计算Id的时间部分，占4字节，即32位，即8个16进行字符。
	time_t timep = time(NULL);
	
	bytes[0] = (unsigned char)(timep >> 24);
	bytes[1] = (unsigned char)(timep >> 16);
	bytes[2] = (unsigned char)(timep >> 8);
	bytes[3] = (unsigned char)(timep);
	
	//随机数播种
	srand((unsigned)time(NULL));
	
	//计算Id的自增部分，占3字节，即24位，即6个16进行字符。
	if(Id_increaseNum == 0){
	    pthread_mutex_lock(&Id_increaseNumLock); //上锁
	    Id_increaseNum = rand();
	    pthread_mutex_unlock(&Id_increaseNumLock); //解锁
	}
	
    pthread_mutex_lock(&Id_increaseNumLock); //上锁
	++Id_increaseNum;
	pthread_mutex_unlock(&Id_increaseNumLock); //解锁
	
	if(Id_increaseNum >= RAND_MAX){
	    pthread_mutex_lock(&Id_increaseNumLock); //上锁
	    Id_increaseNum = 1;
	    pthread_mutex_unlock(&Id_increaseNumLock); //解锁
	}
	
	bytes[5] = (unsigned char)(Id_increaseNum >> 16);
	bytes[6] = (unsigned char)(Id_increaseNum >> 8);
	bytes[7] = (unsigned char)(Id_increaseNum);
	
	
	//叠加式生成随机数，提高随机性
	Id_random = Id_random + rand();
	
	//计算Id的环境部分，占5字节，即40位，即10个16进行字符。
	bytes[8] = (unsigned char)(Id_random >> 24);
	bytes[9] = (unsigned char)(Id_random >> 16);
	bytes[10] = (unsigned char)(Id_random >> 8);
	bytes[11] = (unsigned char)(Id_random);
	
	Id_random = Id_random + rand();
	bytes[12] = (int)(unsigned char)(Id_random);
	
	//转换16进制
	char * chars = (char *)malloc(25); //分配内存
	for(int i = 0; i < 12; i++){
	    char b = bytes[i];
	    chars[i*2] = HEX_CHARS[b >> 4 & 0xF];
	    chars[i*2+1] = HEX_CHARS[b & 0xF];
	}
	
	chars[24] = (char)0;
	
	return chars;
}