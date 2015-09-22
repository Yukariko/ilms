#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv)
{
	if(argc < 4)
	{
		printf("Usage: %s IP ID_START ID_NUM\n",argv[0]);
		return 0;
	}

	FILE *fp = fopen("input.txt","w");
	if(fp == NULL)
	{
		printf("Fopen Fail\n");
		return 0;
	}

	int id_start = atoi(argv[2]);
	int id_num = atoi(argv[3]);

	fprintf(fp, "IP %s\n", argv[1]);
	for(int i=0; i < id_num; i++)
		fprintf(fp, "REG %d\n", id_start + i);

	fclose(fp);
	printf("Input Data Create Success\n");
	return 0;
}