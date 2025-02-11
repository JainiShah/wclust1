// $Id: IsolationDist.cpp,v 1.1 2008/03/19 15:11:03 samn Exp $ 
#include "stdafx.h"
#include "IsolationDist.h"
#include "pca.h"
#include "Cluster.h"
#include "nr\nr.h"
#include "Log.h"
#include <algorithm>
#include "amp\gmpxx.h"
#include "amp\inv.h"
#include "WCMath.h"

using namespace std;

//mpf_class xxx(1.0,500);

typedef double ID_T; //need double for extra precision in matrix operations

void MatMult(vector<vector<ID_T> >& a, vector<vector<ID_T> >& b,vector<vector<ID_T> >& out)
{	int iRowsA = 0, iColsA = 0, iRowsB = 0 , iColsB = 0;
	try
	{
		iRowsA = a.size(); if(!iRowsA) return; iColsA = a[0].size();
		iRowsB = b.size(); if(!iRowsB) return; iColsB = b[0].size();
		if(iColsA!=iRowsB)
			return;
		out = vector<vector<ID_T> >(iRowsA,vector<ID_T>(iColsB));
		int i,j,k;
		for(i=0;i<iRowsA;i++)
			for(j=0;j<iColsB;j++)
				for(k=0;k<iColsA;k++)
					out[i][j] += a[i][k] * b[k][j];
	}
	catch(...)
	{
		Write2Log("Exception in MatMult iRowsA=%d iColsA=%d iRowsB=%d iColsB=%d",iRowsA,iColsA,iRowsB,iColsB);
	}
}

bool Row2Col(vector<vector<float> >& vDataIn,int iRowID,vector<vector<float> >& vDataOut,int iColID)
{
	if(!vDataIn.size())
		return false;
	if(!vDataOut.size())
		return false;
	int iCols = vDataIn[0].size();
	int x;
	for(x=0;x<iCols;x++)
	{
		vDataOut[x][iColID]=vDataIn[iRowID][x];
	}
	return true;
}

bool Transpose(vector<vector<float> >& vDataIn,vector<vector<float> >& vDataOut)
{
	int iRows = vDataIn.size();
	int iCols = vDataIn[0].size();
	if(!iRows || !iCols) return false;
	vDataOut = vector<vector<float> >(iCols,vector<float>(iRows));
	int x,y;
	for(y=0;y<iRows;y++)
	{	for(x=0;x<iCols;x++)
		{
			vDataOut[x][y]=vDataIn[y][x];
		}
	}
	return true;
}

bool InvertMatrix(vector< vector<ID_T> >& vData)
{
	//ID_T d;
	//vector<vector<ID_T> > vTmp(vData);
	//int i,j;
	int N = vData.size();
	/*vector<int> indx(N);
	vector<ID_T> col(vData[0].size());
	if(!NR::ludcmp(vTmp,indx,d))//Decompose the matrix just once.
	{	Write2Log("Couldn't invert matrix!");
		return false;		
	}
	for(j=0;j<N;j++) 
	{ //Find inverse by columns.
		for(i=0;i<N;i++)
			col[i]=0.0;
		col[j]=1.0;
		NR::lubksb(vTmp,indx,col);
		for(i=0;i<N;i++) 
			vData[i][j]=col[i];
	}*/	

	ap::template_2d_array< amp::ampf<500> > a;
	a.setbounds(1,N,1,N);
	int x,y;
	for(y=1;y<=N;y++)
		for(x=1;x<=N;x++)
			a(y,x)=vData[y-1][x-1];

	if(!inv::inverse(a,N))
		return false;

	Write2Log("a(1,1)=%s",a(1,1).toDec().c_str());

	for(y=1;y<=N;y++)
		for(x=1;x<=N;x++)
			vData[y-1][x-1]=a(y,x).toDouble();

	return true;
}

// Variance-covariance matrix: creation
void CovarMat(vector< vector<ID_T> >& vDataIn,int iRows,int iCols,vector< vector<ID_T> >& vCovarMat, vector<ID_T>& vMean)
{	// Create iCols * iCols covariance matrix from given iRow * iCol data matrix. 
	int i, j, x2, x, y, j1 , j2;
	// Allocate storage for mean vector 
	vMean = vector<ID_T>(iCols);

	vector< vector<ID_T> > vData = vDataIn; // copy it!

	// Determine mean of row vectors of input data matrix 
	for(x=0;x<iCols;x++)
	{	vMean[x] = 0.0;
		for(y=0;y<iRows;y++)
		{	vMean[x] += vData[y][x];
		}
		vMean[x] /= (ID_T)iRows;
	}

	vector<ID_T> vStdev(iCols);
	
	// Center the row vectors.
	for(y=0;y<iRows;y++)
    {	for(x=0;x<iCols;x++)
		{	vData[y][x] -= vMean[x];
		}
    }

	// compute std-dev
	const bool bCorrelMat = false;
	if(bCorrelMat) for(x=0;x<iCols;x++)
	{	for(y=0;y<iRows;y++)
			vStdev[x]+=vData[y][x]*vData[y][x];
		vStdev[x] /= (ID_T) iRows;
		vStdev[x] = sqrt(vStdev[x]);
	}

	vCovarMat = vector<vector<ID_T> >(iCols,vector<ID_T>(iCols));

	// Calculate the iCols * iCols covariance matrix.
	for(j1=0;j1<iCols;j1++)
	{	for(j2=0;j2<=j1;j2++)
		{	vCovarMat[j1][j2]=0.0;
			for(i=0;i<iRows;i++)
			{	vCovarMat[j1][j2] += (vData[i][j1] * vData[i][j2]);
			}
			if(bCorrelMat)	//correlation matrix
				vCovarMat[j1][j2] /= ((ID_T) iRows * vStdev[j1] * vStdev[j2] );
			else	//covariance matrix
				vCovarMat[j1][j2] /= (ID_T) (iRows); // -1);
			vCovarMat[j2][j1]=vCovarMat[j1][j2];
		}
	}
}

//gets the inverse of the covariance matrix for a cluster
//input vFloat has each row as a data vector, each column as a dimension, to get data vector i do vFloat[i*iCols]
bool Clust2CovarMatInv(vector<vector< ID_T > >& vCovarMat,vector<ID_T>& vMean, vector<int>& vClustIDs,int iClustID,vector<float >& vFloat,int iRows,int iCols,int& iClustSz)
{
	int i = 0, j = 0;
		iClustSz = count(vClustIDs.begin(),vClustIDs.end(),iClustID);
	if(!iClustSz)
		return false;
	vector< vector<ID_T> > vClustData; 
	vClustData = vector<vector<ID_T> >(iClustSz,vector<ID_T>(iCols));
	for(i=0;i<iRows;i++)
	{	if(vClustIDs[i]==iClustID)
		{	copy(&vFloat[i*iCols],&vFloat[i*iCols+iCols],&vClustData[j++][0]);
		}
	}
	CovarMat(vClustData,iClustSz,iCols,vCovarMat,vMean);
	
//#ifdef _DEBUG
	vector<vector<ID_T> > mtmp(vCovarMat);
	
	Write2Log("covar mat");
	WriteMat2Log(vCovarMat);
	
	bool b = InvertMatrix(vCovarMat);

	Write2Log("inv covar mat");
	WriteMat2Log(vCovarMat);
	
	vector<vector<ID_T> > vident;
	MatMult(mtmp,vCovarMat,vident);
	
	Write2Log("is this identity matrix?");
	WriteMat2Log(vident);
	
	return b;
//#else
//	return InvertMatrix(vCovarMat);
//#endif
}

ID_T DotProd(vector<ID_T>& v1,vector<ID_T>& v2)
{
	int iSz = v1.size(), i = 0;
	ID_T val = 0.0;
	for(i=0;i<iSz;i++) val += v1[i]*v2[i];
	return val;
}

template< int P > 
ID_T DotProd(vector<amp::ampf<P> >& v1,vector<amp::ampf<P> >& v2)
{
	int iSz = v1.size(), i = 0;
	amp::ampf<P> val;
	for(i=0;i<iSz;i++) val += v1[i]*v2[i];
	return val.toDouble();
}

//squared mahalanobis distance of pData to vMean using inverse of covariance matrix, vCovarMatInv
ID_T MahalDistSQ(float* pData,int iRowSz,vector< vector<ID_T> >& vCovarMatInv, vector<ID_T>& vMean)
{	// (x-mean)'*covar^-1*(x-mean)
	vector<ID_T> vRow(iRowSz,0.0), vTmp(iRowSz,0.0);
	int i, iCols = iRowSz , j = 0;
	for(i=0;i<iCols;i++)	//center row data vector
		vRow[i]=pData[i] - vMean[i];
	
	//multiply vector by matrix
	for(i=0;i<iCols;i++)
		for(j=0;j<iCols;j++)
			vTmp[i] += vRow[j] * vCovarMatInv[j][i];

	//dot product
	return DotProd(vTmp,vRow);
}

//squared mahalanobis distance of pData to vMean using inverse of covariance matrix, vCovarMatInv
template< int P >
ID_T MahalDistSQ(float* pData,int iRowSz,ap::template_2d_array< amp::ampf<P> >& vCovarMatInv, vector<ID_T>& vMean)
{	// (x-mean)'*covar^-1*(x-mean)
	vector<amp::ampf<P> > vRow(iRowSz), vTmp(iRowSz);
	int i, iCols = iRowSz , j = 0;
	for(i=0;i<iCols;i++)	//center row data vector
		vRow[i]=pData[i] - vMean[i];
	
	//multiply vector by matrix
	for(i=0;i<iCols;i++)
		for(j=0;j<iCols;j++)
			vTmp[i] += vRow[j] * vCovarMatInv(j,i);

	//dot product
	return DotProd(vTmp,vRow);
}

float IsolationDist(vector<int>& vClustIDs,int iClustID,vector<float >& vFloat,int iRows,int iCols)
{
	vector<vector<ID_T> > vCovarMatInv;
	vector<ID_T> vMean;
	int iClustSz = 0;
	if(!Clust2CovarMatInv(vCovarMatInv,vMean,vClustIDs,iClustID,vFloat,iRows,iCols,iClustSz))
		return -1.0;
	if(iClustSz*2>=iRows) //need at least as many points outside of cluster as within it
		return -1.0;

	//Write2Log("vMean for cluster %d : ",iClustID); WriteVec2Log(vMean);

	/*ap::template_2d_array< amp::ampf<200> > a;
	a.setbounds(0,iCols,0,iCols);
	int x,y;
	for(y=0;y<iCols;y++)
		for(x=0;x<iCols;x++)
			a(y,x)=vCovarMatInv[y][x];*/

	vector<ID_T> vDists; 
	int i;
	for(i=0;i<iRows;i++)
	{
		if(vClustIDs[i]==iClustID)
			continue;
		ID_T dDist = MahalDistSQ(&vFloat[i*iCols],iCols,vCovarMatInv,vMean);
		//ID_T dDist = MahalDistSQ(&vFloat[i*iCols],iCols,a,vMean);
		if(dDist>=0.0){
			vDists.push_back(dDist);
			if(dDist>=1e200)
				Write2Log("large val = %g",dDist);
		}
		else
			Write2Log("Negative MahalDistSq = %.4f",dDist);
	}

	sort(vDists.begin(),vDists.end());

	Write2Log("meandist=%.4f mediandist=%.4f",Avg(vDists),vDists[vDists.size()/2]);

	if(iClustSz-1>=0 && iClustSz-1<vDists.size())
		return vDists[iClustSz-1];
	return -1.0;
}
