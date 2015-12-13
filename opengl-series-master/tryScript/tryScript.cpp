// tryScript.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <new>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace std;

//
template <typename T>
T** new_Array2D(int row, int col)
{
	int size = sizeof(T);
	int point_size = sizeof(T*);
	//
	T **arr = (T **)malloc(point_size * row + size * row * col);
	if (arr != NULL)
	{
		T *head = (T*)((int)arr + point_size * row);
		for (int i = 0; i < row; ++i)
		{
			arr[i] = (T*)((int)head + i * col * size);
			for (int j = 0; j < col; ++j)
				new (&arr[i][j]) T;
		}
	}
	return (T**)arr;
}
//release
template <typename T>
void delete_Array2D(T **arr, int row, int col)
{
	for (int i = 0; i < row; ++i)
		for (int j = 0; j < col; ++j)
			arr[i][j].~T();
	if (arr != NULL)
		free((void**)arr);
}
int _tmain(int argc, _TCHAR* argv[])
{
	printf(" allocate new delete\n");

	printf("row and col: ");
	int nRow, nCol;
	scanf("%d %d", &nRow, &nCol);


	//string **p = new_Array2D<string>(nRow, nCol);

	////value
	//int i, j;
	//for (i = 0; i < nRow; i++)
	//	for (j = 0; j < nCol; j++)
	//	{
	//	char szTemp[30];
	//	sprintf(szTemp, "(the %d row,the %d col)", i, j);
	//	p[i][j] = szTemp;
	//	}

	////printf	
	//for (i = 0; i < nRow; i++)
	//{
	//	for (j = 0; j < nCol; j++)
	//		printf("%s ", p[i][j].c_str());
	//	putchar('\n');
	//}
	//float ** p = new_Array2D<float>(nRow, nCol);
	////value
	//int i, j;
	//for (i = 0; i < nRow; i++)
	//{
	//	for (j = 0; j < nCol; j++)
	//	{
	//		p[i][j] = (float)i*j;
	//	}
	//}
	////printf	
	//for (i = 0; i < nRow; i++)
	//{
	//	for (j = 0; j < nCol; j++)
	//		printf("%f ", p[i][j]);
	//	putchar('\n');
	//}

	//delete_Array2D<float>(p, nRow, nCol);

	// 3 dimension
	int ***array3D;
	printf("row col height \n");
	int m,n,h;
	//scanf("%d %d %d", &m, &n,&h);
	m = 2; n = 3, h = 4;
	array3D = new int **[m];
	for (int i = 0; i<m; i++)
	{
		array3D[i] = new int *[n];
		for (int j = 0; j<n; j++)
		{
			array3D[i][j] = new int[h];
		}
	}

	for (int i = 0; i<m; i++)
	{
		for (int j = 0; j<n; j++)
		{
			for (int k = 0; k < h; k++)
			{
				array3D[i][j][k] = ((float)i)*j*k;
				printf("%d %d %d %f ", i, j, k, ((float)i)*j*k);
				putchar('\n');
			}
		}
	}

	for (int i = 0; i<m; i++)
	{
		for (int j = 0; j<n; j++)
		{
			for (int k = 0; k < h; k++)
			{
				printf("%d %d %d %f ", i,j,k,array3D[i][j][k]);
				putchar('\n');
			}
		}
	}

	// relase
	for (int i = 0; i<m; i++)
	{
		for (int j = 0; j<n; j++)
		{
			delete array3D[i][j];
		}
		delete array3D[i];
	}
	delete array3D;
	getchar();
	return 0;
}


