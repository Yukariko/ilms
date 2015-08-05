#include <cstdio>
#include <cstdlib>

int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		printf("Usage: %s VFNUM\n",argv[0]);
		return 0;
	}

	FILE *fp = fopen("tree.conf","w");
	fprintf(fp, "0\n");
	fprintf(fp, "%s\n", argv[1]);

	int num = atoi(argv[1]);
	for(int i=1; i < num; i++)
		fprintf(fp, "0\n");
	fprintf(fp, "210.117.184.166\n0\n0\n");
	return 0;
}
