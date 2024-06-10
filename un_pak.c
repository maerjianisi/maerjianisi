#include <stdio.h>
#include <stdlib.h>
#include <io.h>	//_access()
#include <direct.h>	//_mkdir()

typedef unsigned char byte_t;	//“字节”数据类型

//链表节点，用于存储文件信息
struct FileInfoNode;
typedef struct FileInfoNode FileInfoNode;
struct FileInfoNode{
	char * pPathname;	//文件名（含路径）指针
	unsigned int fileSize;	//文件长度（需要保证编译器上的sizeof (unsigned int) == 4）
	FileInfoNode * pNext;	//后节点的指针
};

int main(void){
	//程序运行开始
	printf("===Welcome!===\n");

	//尝试打开“main.pak”
	FILE * fpMainPak = fopen("main.pak", "rb");
	if (NULL == fpMainPak){
		fprintf(stderr, "ERROR : Failed to open file \"main.pak\".\n");
		exit(EXIT_FAILURE);
	}

	//成功打开，获取文件长度
	fseek(fpMainPak, 0, SEEK_END);
	long sizeMainPak = ftell(fpMainPak);
	rewind(fpMainPak);

	//文件长度必须为正整数
	if ( !(sizeMainPak > 0) ){
		fprintf(stderr, "ERROR : Size of file \"main.pak\" must be greater than zero.\n");
		exit(EXIT_FAILURE);
	}

	//将“main.pak”读入内存
	printf("Loading \"main.pak\"...");
	byte_t * pMainPak = (byte_t *)malloc(sizeMainPak);
	if (NULL == pMainPak){
		fprintf(stderr, "ERROR : Failed to allocate memory for the memory image of file \"main.pak\".\n");
		exit(EXIT_FAILURE);
	}
	if (fread(pMainPak, sizeof (byte_t), sizeMainPak, fpMainPak) != sizeMainPak){
		fprintf(stderr, "ERROR : Failed to load file \"main.pak\" to memory.\n");
		exit(EXIT_FAILURE);
	}
	printf("[Finished!]\n");

	//已将“main.pak”读入内存，关闭该文件
	fclose(fpMainPak);
	fpMainPak = NULL;

	//从这里开始，使用pMainPak（首字节指针）和sizeMainPak（字节数）操作

	//打包文件逐字节使用密钥0xF7通过异或法做了加密，需要先进行解密
	printf("Decrypting data...");
	for (int i = 0; i < sizeMainPak; i += 1)
		pMainPak[i] ^= (byte_t)0xF7;
	printf("[Finished!]\n");

	//创建一条链表，用于存储打包文件中的文件信息
	FileInfoNode headNode;
	headNode.pNext = NULL;
	//创建链表节点的指针（初始时指向头节点），用于操作该链表
	FileInfoNode * pNode = &headNode;

	//创建字节索引，表示当前正在操作的字节号（初始时操作首字节）
	unsigned int byteIndex = 0;

	//读取打包文件中的文件信息（循环一次读取一个文件）
	printf("Scanning files...");
	while (1){
		//跳过前8个字节（这8个字节的作用尚不明确），指向第9个字节
		byteIndex += 8;
		//检测第9个字节是否为0，不是则说明读取结束
		if (0 != pMainPak[byteIndex]){
			//读取结束时，让索引指向其后一个字节，此处是数据段的开始
			byteIndex += 1;
			break;
		}

		//没有break，说明有新文件，为其创建一个新的链表节点
		pNode->pNext = (FileInfoNode *)malloc(sizeof (FileInfoNode));
		pNode = pNode->pNext;
		if (NULL == pNode){
			fprintf(stderr, "ERROR : Failed to allocate memory for new FileInfoNode.\n");
			exit(EXIT_FAILURE);
		}
		pNode->pNext = NULL;

		//索引移动到第10个字节，该字节表示文件名（含路径）的字符数
		byteIndex += 1;
		//为文件名分配内存（为'\0'额外分配1字节内存）
		pNode->pPathname = (char *)malloc(pMainPak[byteIndex] + 1);
		if (NULL == pNode->pPathname){
			fprintf(stderr, "ERROR : Failed to allocate memory for Pathname linked to FileInfoNode.\n");
			exit(EXIT_FAILURE);
		}
		//字符串'\0'封尾
		(pNode->pPathname)[pMainPak[byteIndex]] = '\0';
		//复制文件名
		for (int i = 0; i < pMainPak[byteIndex]; i += 1)
			(pNode->pPathname)[i] = pMainPak[byteIndex + 1 + i];

		//索引移动到文件名后的第1字节，该处的4个字节表示文件长度
		byteIndex += 1 + pMainPak[byteIndex];
		//读取文件长度
		pNode->fileSize = *((unsigned int *)(pMainPak + byteIndex));

		//索引移动到文件长度后的第1字节，此处是下一段数据的起点
		byteIndex += 4;
	}
	printf("[Finished!]\n");

	//此时，所有文件信息均已被读取到链表中

	//遍历链表，计算所有文件的长度和，并比较是否与数据段的真实长度一致（这样在导出文件时就不需要担心数组越界）
	{
		unsigned int totalSize = 0;
		for (pNode = headNode.pNext; pNode != NULL; pNode = pNode->pNext)
			totalSize += pNode->fileSize;
		if (totalSize != sizeMainPak - byteIndex){
			fprintf(stderr, "ERROR : Size of file \"main.pak\" is unexpected.\n");
			exit(EXIT_FAILURE);
		}
	}

	//所有准备就绪，开始导出文件！（每次循环导出一个）
	printf("Releasing files(Please wait patiently)...");
	for (pNode = headNode.pNext; pNode != NULL; pNode = pNode->pNext){
		//先确认路径有效性（也就是确认文件所在的文件夹已经创建好了，没有创建好则创建）
		{
			//指向反斜杠的指针（初始时指向第一个字符）
			char * pSlash = pNode->pPathname;
			while (1){
				//找到下一个'\\'或者'\0'
				while ( !(*pSlash == '\\' || *pSlash == '\0') )
					pSlash += 1;
				//如果是'\0'，则路径有效性已得到确认，结束循环
				if ('\0' == *pSlash)
					break;

				//此时是'\\'，需要进行路径有效性确认

				//先将'\\'换为'\0'
				*pSlash = '\0';
				//确认有效性
				if (-1 == _access(pNode->pPathname, 0))//F_OK
					_mkdir(pNode->pPathname);
				//确认完毕，还原
				*pSlash = '\\';

				//因为本轮检测已经完成，所以跳过本次的'\\'，进行下一轮
				pSlash += 1;
			}
		}

		//打开待写入的文件
		FILE * fpRelease = fopen(pNode->pPathname, "wb");
		if (NULL == fpRelease){
			fprintf(stderr, "ERROR : Failed to open file to be written.\n");
			exit(EXIT_FAILURE);
		}

		//打开成功，开始写入
		if (fwrite(pMainPak + byteIndex, sizeof (byte_t), pNode->fileSize, fpRelease) != pNode->fileSize){
			fprintf(stderr, "ERROR : An error occurred while writting.\n");
			exit(EXIT_FAILURE);
		}

		//写入结束，关闭文件
		fclose(fpRelease);
		fpRelease = NULL;

		//索引后移到下一个文件的首字节
		byteIndex += pNode->fileSize;
	}
	printf("[Finished!]\n");

	//链表使用结束，释放整条链表
	pNode = headNode.pNext;
	headNode.pNext = NULL;
	while (pNode != NULL){
		FileInfoNode * pNextNode = pNode->pNext;
		free(pNode->pPathname);
		free(pNode);
		pNode = pNextNode;
	}

	//释放“main.pak”的memory image
	free(pMainPak);
	pMainPak = NULL;

	//程序成功结束
	printf("===Success!===\n");
	return 0;
}