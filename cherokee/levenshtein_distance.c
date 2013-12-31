/*
 * Copyright 2002 by Steven McDougall. This module is free
 * software; you can redistribute it and/or modify it under the same
 * terms as Perl itself.
 *
 */

#include "common-internal.h"
#include "levenshtein_distance.h"

#include <stdlib.h>
#include <string.h>


static int  _prefix_distance(char *A, char *B, int lA, int lB);
static int  _min(int a, int b, int c);
static void score(int *pD, char *A, char *B, int lA, int lB);

#define D(r,c) pD[(r)*lB1+(c)]

int distance(char *A, char *B)
{
	int *pD, d;

	int lA  = strlen(A);
	int lB  = strlen(B);

	int lA1 = lA+1;
	int lB1 = lB+1;

	pD = (int *) malloc(lA1 * lB1 * sizeof(int));
	if (!pD)
		return -1;

	score(pD, A, B, lA, lB);

	d = D(lA,lB);
	free(pD);

	return d;
}


int prefix_distance(char *A, char *B)
{
	int lA  = strlen(A);
	int lB  = strlen(B);

	return lA < lB ?
	    _prefix_distance(A, B, lA, lB) :
	    _prefix_distance(B, A, lB, lA);
}


static int _prefix_distance(char *A, char *B, int lA, int lB)
{
	int *pD, c, d;

	int lA1 = lA+1;
	int lB1 = lB+1;

	pD = (int *) malloc(lA1 * lB1 * sizeof(int));
	if (!pD)
		return -1;

	score(pD, A, B, lA, lB);

	d = D(lA, lA);
	for (c = lA+1; c <= lB; c++)
		if (d > D(lA, c))
			d = D(lA, c);

	free(pD);

	return d;
}


static void score(int *pD, char *A, char *B, int lA, int lB)
{
	int r, c;

	int lB1 = lB+1;

	for (r = 0; r <= lA; r++)
		D(r,0) = r;

	for (c = 0; c <= lB; c++)
		D(0,c) = c;

	for (r = 1; r <= lA; r++) {
		for (c = 1; c <= lB; c++) {
			int u    = D(r-1,c  );
			int l    = D(r  ,c-1);
			int ul   = D(r-1,c-1);
			int cost = (A[r-1] == B[c-1] ? 0 : 1);
			D(r,c)   = _min(u+1, l+1, ul+cost);
		}
	}
}


static int _min (int a, int b, int c)
{
	int min = a;

	if (min > b)
		min = b;
	if (min > c)
		min = c;

	return min;
}


