#include <cstdio>
#include <cstdlib>

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		printf("Usage: %s VFNUM CHILDNUM\n",argv[0]);
		return 0;
	}

	FILE *fp = fopen("tree.conf","w");
	if(fp == NULL)
	{
		printf("Fopen Fail\n");
		return 0;
	}
	int vf_num = atoi(argv[1]);
	int child_num = atoi(argv[2]);

	fprintf(fp, "0\n");
	fprintf(fp, "%d\n", vf_num + child_num);

	for(int i=0; i < vf_num; i++)
		fprintf(fp, "0\n");

	for(int i=0; i < child_num; i++)
	{
		char buf[256];
		printf("Input child %d's IP : ", i+1);
		scanf("%s",buf);
		fprintf(fp, "%s\n", buf);
	}

	fprintf(fp, "0\n0\n");
	fclose(fp);

	printf("tree.conf Create Success\n");
	return 0;
}
