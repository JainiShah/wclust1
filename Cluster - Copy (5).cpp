// $Id: Cluster.cpp,v 1.40 2008/07/28 18:14:25 samn Exp $ 
// Cluster.cpp: implementation of the CCluster class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <hash_map>
#include <float.h>
#include <io.h>
#include "wclust.h"
#include "Cluster.h"
#include "Vertex.h"
#include "Hist.h"
#include <algorithm>
#include <map>
#include "apcluster.h"
#include "KlustaKwik\KlustaKwik.h"
#include "CDNAClust\cluster.h"
#include "KDBTree\KDBTree.h"
#include "Log.h"
#include "MyVers.h"
#include "IsolationDist.h"
#include "InfoT.h"
#include "Silhouette.h"

using namespace std;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CCluster
CCluster::CCluster()
{ 
	undoBndry=0;//new CBoundary();//0;
	m_Count=0; 
	m_ClMode=2;
	CBoundary *Bnd;
	Bnd=new CBoundary(); 
	AddBndry(Bnd); 
	m_newChanges = 0;
	m_Swap = 0;
	memset(m_pNumClusts,0,sizeof(int)*6);
	
	m_bUsePCA = false;
	m_bCombinePCA = true;
	m_iNumPCsToStore = 0;
	m_iPCAStartDim = -1;
	m_iPCAEndDim = -1;

	m_iNNToFind = 1;
	m_bFastKLD = false;
	m_bInterKLD = true;
	m_bDoKLDiv = true;
	m_bIsolationDist = false;
	m_iBestDims = 8;
	m_bFindBestDims = true;
	m_b1DBest = false; //use 2D by default
	m_bSilhouette = false;
};

CCluster::CCluster(CPaletteStack *MyPalette)
{ 
	undoBndry=0;
	m_PalStack = MyPalette; 
	m_Count = 0; 
	m_ClMode = 1;
	CBoundary *Bnd;
	Bnd=new CBoundary(MyPalette); 
	AddBndry(Bnd); 
	m_newChanges = 0;
	m_Swap = 0;
	memset(m_pNumClusts,0,sizeof(int)*6);

	m_bUsePCA = false;
	m_bCombinePCA = true;
	m_iNumPCsToStore = 0;
	m_iPCAStartDim = -1;
	m_iPCAEndDim = -1;

	m_iNNToFind = 1;
	m_bFastKLD = false;
	m_bInterKLD = true;
	m_bDoKLDiv = true;
	m_bIsolationDist = false;
	m_iBestDims = 8;
	m_bFindBestDims = true;
	m_b1DBest = false; //use 2D by default
	m_bSilhouette = false;
};

//////////////////////////////////////////////////////////////////////
void CCluster::SetEmpty()
{
	MY_STACK::iterator Index;
	CBoundary *Bndr;

	for (Index=m_BndrStack.begin();Index!=m_BndrStack.end();Index++)
	{
		Bndr = (CBoundary*) *Index;
		Bndr->SetEmpty();
		delete Bndr;
	}
	m_BndrStack.clear();
	
	CParamDim *pd;
	for (Index=m_ParamDimStack.begin();Index!=m_ParamDimStack.end();Index++)
	{
		pd = (CParamDim*)*Index;
		delete pd;
	}
	m_ParamDimStack.clear();

	usedExternWCP.clear();

	m_pNumClusts[CLUST_USER] = m_Count = 0;
	m_newChanges = 0;
	ClearUNDO();

	m_bUsePCA = false;
	m_bCombinePCA = false;
	m_iNumPCsToStore = 0;
	m_iPCAStartDim = -1;
	m_iPCAEndDim = -1;
};

//////////////////////////////////////////////////////////////////////
void CCluster::Clear()
{
	MY_STACK::iterator Index;
	CBoundary *Bndr;
	m_pNumClusts[CLUST_USER] = m_Count=0;

	for (Index=m_BndrStack.begin();Index!=m_BndrStack.end();Index++)
	{
		Bndr = (CBoundary*) *Index;
		Bndr->SetEmpty();
		delete Bndr;
	}
	m_BndrStack.clear();

	
	CParamDim *pd;
	for (Index=m_ParamDimStack.begin();Index!=m_ParamDimStack.end();Index++)
	{
		pd = (CParamDim*)*Index;
		delete pd;
	}
	m_ParamDimStack.clear();

	Bndr=new CBoundary(); 
	AddBndry(Bndr);

	m_bUsePCA = false;
	m_bCombinePCA = false;
	m_iNumPCsToStore = 0;
	m_iPCAStartDim = -1;
	m_iPCAEndDim = -1;
}

//////////////////////////////////////////////////////////////////////
void CCluster::SwapPolarity(CVerxStack *Pnts)
{
	if (m_Swap)
	{
		m_Swap = 0;
	}
	else{
		m_Swap = 1;
	}
	Pnts->SwapPolarity();
	CalcParam(Pnts);
	Pnts->CalcMinMax();
	Clustering(Pnts);
	m_newChanges = 3;
}

//////////////////////////////////////////////////////////////////////
void CCluster::AddAxes(CVerxStack *Pnts,CParamDim *param)
{
	if ( param->GetType() == 2 )
	{  
		CString *MyText = new CString("T-V()");
		char pom[64],chanel[64];
		sprintf(chanel,"%d",(int) param->GetParam(2)+1);
		MyText->Insert(1,chanel);
		sprintf(pom,"%1.3f",param->GetParam(1)*1000);
		for (int i=0;i<5;i++)
		{
			MyText->Insert(5+i,pom[i]);
		}
		Pnts->AddAxes(MyText);
	}
	else if ( param->GetType() == 3 )
	{
		CString *MyText = new CString("T-dY()");
		char pom[64],chanel[64];
		sprintf(chanel,"%d",(int) param->GetParam(2)+1);
		MyText->Insert(1,chanel);
		sprintf(pom,"%1.3f",param->GetParam(1)*1000);
		for (int i=0;i<5;i++)
		{
			MyText->Insert(6+i,pom[i]); 
		}
		Pnts->AddAxes(MyText);
	}
	else if ( param->GetType() == 4 )
	{
		CString *MyText = new CString("T-E(-)");
		char pom[64],chanel[64];
		sprintf(chanel,"%d",(int) param->GetParam(3)+1);
		MyText->Insert(1,chanel);
		sprintf(pom,"%1.3f",param->GetParam(1)*1000);
		for (int i=0;i<5;i++)
		{
			MyText->Insert(5+i,pom[i]);
		}
		sprintf(pom,"%1.3f",param->GetParam(2)*1000);
		for (i=0;i<5;i++)
		{
			MyText->Insert(11+i,pom[i]);
		}
		Pnts->AddAxes(MyText);
	}
	else if ( param->GetType() == 5 )
	{	// Parameter from file

		CString *MyText = new CString("T-");
	
		MyText->Insert(2, (LPCTSTR)  (usedExternWCP.begin() + (int) param->GetParam(1) )->paramName );
	
		char chanel[64];
		sprintf(chanel,"%d",(int) param->GetParam(2)+1);
	
		MyText->Insert(1,chanel);
	
		Pnts->AddAxes(MyText);	
	}
	else if ( param->GetType() == 6 )
	{	// Cat  Total  ID
		int isz = param->m_Par2 , i = 0;
		for(i=0;i<isz;i++)
		{
			CString *MyText = new CString();
			MyText->Format("W-%d",i+1);
			Pnts->AddAxes(MyText);
		}
	}
	else if ( param->GetType() == 7 ) //peak symmetry
	{
		CString *MyText = new CString();
		MyText->Format("T%d-PSym",1+(int)param->GetParam(3));
		Pnts->AddAxes(MyText);
	}
	else if ( param->GetType() == 8 ) //valley symmetry
	{
		CString *MyText = new CString();
		MyText->Format("T%d-VSym",param->GetParam(3));
		Pnts->AddAxes(MyText);
	}
}

//////////////////////////////////////////////////////////////////////
void CCluster::AddParam(CMyObject *toStore, CVerxStack *Pnts, bool bUserAdd)
{
	m_ParamDimStack.push_back(toStore); 

	if(bUserAdd)
	{
		Pnts->Dimension += 1;
		Pnts->BASE_DIMENSION += 1;

		CParamDim* param = (CParamDim*) toStore; 
		param->m_iVDim = Pnts->Dimension;
			
		if((param->GetType() >= 2 && param->GetType() <= 5) ||
			(param->GetType() == 6 && param->m_Par3 == 0) ||
			param->GetType() == 7 || param->GetType() == 8
			)
		{
			AddAxes(Pnts,param);
			CalcParamLast(Pnts);
			Pnts->CalcMinMaxLast();
		}
	}

	m_newChanges = 3;
}

//////////////////////////////////////////////////////////////////////
void CCluster::RemParam(CVerxStack *Pnts, int iNum)
{
	MY_STACK::iterator indexPD = m_ParamDimStack.begin() + iNum;
	CParamDim* parDim = (CParamDim*) *indexPD;

	MY_STACK::iterator indexBndry;
	for (indexBndry = m_BndrStack.begin();indexBndry != m_BndrStack.end(); indexBndry++)
	{
		CBoundary* bndry = (CBoundary*) *indexBndry;
		bndry->AdaptProj(parDim->m_iVDim);//remove projection associated with indexPD
	}	
	
	if ( parDim->GetType() == 5 )
	{	// external param from file
		
		// first check, if there is more than 1 param of the candidat to delete
		int sameParams = 0;
		while ( indexPD != m_ParamDimStack.end() )
		{
			if ( ((CParamDim*)*indexPD)->GetType() == 5 )
			{
				if ( ((CParamDim*)*indexPD)->GetParam(1) == parDim->GetParam(1) )
				{
					sameParams++;
				}
			}
			indexPD++;
		}
		if ( sameParams == 1 )
		{
			usedExternWCP.erase( usedExternWCP.begin() + (int) parDim->GetParam(1) );
		}

		indexPD = m_ParamDimStack.begin() + iNum;

		Pnts->RemoveExternalParam( (int) parDim->GetParam(3) );
		while ( indexPD != m_ParamDimStack.end() )
		{
			if ( ((CParamDim*)*indexPD)->GetType() == 5 )
			{
				((CParamDim*)*indexPD)->SetParam(3, ((CParamDim*)*indexPD)->GetParam(3) - 1);
				if ( sameParams == 1 )
				{
					((CParamDim*)*indexPD)->SetParam(1, ((CParamDim*)*indexPD)->GetParam(1) - 1);
				}
			}
			indexPD++;
		}
		indexPD = m_ParamDimStack.begin() + iNum;
	}

	delete parDim;
	m_ParamDimStack.erase(indexPD);
	m_newChanges = 3;
}

//////////////////////////////////////////////////////////////////////
void CCluster::RemParamZeros(CVerxStack *Pnts)
{
	int paramNo = 0;
	
	CParamDim *param;
	MY_STACK::iterator indexPD;
	indexPD = m_ParamDimStack.begin();
	while ( indexPD != m_ParamDimStack.end() )
	{
		param = (CParamDim*)*indexPD;
		if ( param->m_Type == 5 )
		{
			sWcpFiles *wcp = &*( usedExternWCP.begin() + (int)param->GetParam(1) );
			if ( wcp->fileName.Compare( "-" ) == 0 )
			{
				RemParam(Pnts, paramNo);
				RemParam(Pnts, paramNo);
				RemParam(Pnts, paramNo);
				RemParam(Pnts, paramNo);
				indexPD = m_ParamDimStack.begin(); paramNo = 0;
			}
			else {
				indexPD++;
				paramNo++;
			}
		}
		else {
			indexPD++;
			paramNo++;
		}
	}
	ClearUNDO();
}

//////////////////////////////////////////////////////////////////////
void CCluster::Draw(CDC *pDC,CRect DrawWin,CFloatRect *whichValues,CPoint whichAxes,unsigned char *Clust,int style)
{
	MY_STACK::iterator Index;
	for (Index=m_BndrStack.begin();Index!=m_BndrStack.end();Index++)
	{
		(*Index)->Draw(pDC,DrawWin,whichValues,whichAxes,Clust,style);
	}
}

//////////////////////////////////////////////////////////////////////
void CCluster::SetNoise(CVerxStack *Pnts)
{
	if(!m_BndrStack.size()) return;

	//only set to noise the vertices in the most recently added boundary
	//(the one just made with the noise button)
	MY_STACK::iterator iBndry=m_BndrStack.end();
	iBndry--;
	CBoundary* Bndr = (CBoundary*) *iBndry;
	MY_STACK::iterator iPnts;
	CVertex *m_vx;

	for (iPnts = Pnts->m_VerxStack.begin(); iPnts != Pnts->m_VerxStack.end();iPnts++)
	{
		m_vx = (CVertex*)*iPnts;
		//m_vx->EmptyClust();
		//m_vx->AddClust(0);

		//if ( m_vx->GetNoise() )
		{
		//	m_vx->AddClust(255);
		}
		//else
		{	

			//Bndr->Clustering(m_vx);
			if(Bndr->IsIn(m_vx))
			{
				//m_vx->AddClust(255); //why does that cause a crash in release mode??
					                    //when prob_t clicking on edit window after noise
										//points have been added...??
				m_vx->SetNoise(1);
			}
			else
			{
				int moo=0;
			}	
			//m_vx->SelectCluster(m_ClMode);
		}
	}
	//recalculate min,max so that noise will not contribute
	Pnts->CalcMinMax();
}

//////////////////////////////////////////////////////////////////////
void CCluster::Clustering(CVerxStack *Pnts,bool NoiseOn)
{
	MY_STACK::iterator iPnts,iBndry;
	CBoundary *Bndr;
	CVertex *m_vx;

	for (iPnts = Pnts->m_VerxStack.begin(); iPnts != Pnts->m_VerxStack.end();iPnts++)
	{
		m_vx = (CVertex*)*iPnts;
		m_vx->SetNoise(false);
	}

	vector<bool> vNoise(m_BndrStack.size()); int iB = 0;
	for (iBndry = m_BndrStack.begin(); iBndry!=m_BndrStack.end(); iBndry++,iB++)
	{
		Bndr = (CBoundary*) *iBndry;
		vNoise[iB]=Bndr->m_bNoise;
	}

	for (iPnts = Pnts->m_VerxStack.begin(); iPnts != Pnts->m_VerxStack.end();iPnts++)
	{
		m_vx = (CVertex*)*iPnts;
		m_vx->EmptyClust();
		m_vx->AddClust(0);

		//if ( m_vx->GetNoise() )
		{
		//	m_vx->AddClust(255);
		}
		//else
		{	
			for (iBndry = m_BndrStack.begin(); iBndry!=m_BndrStack.end(); iBndry++)
			{
				Bndr = (CBoundary*) *iBndry;
				Bndr->Clustering(m_vx,NoiseOn);
			}
	
			m_vx->SelectCluster(m_ClMode,vNoise);
		}
	}
	//recalculate min,max so that noise will not contribute
	Pnts->CalcMinMax();
}

//////////////////////////////////////////////////////////////////////
void CCluster::Clustering(CVertex *Pnt)
{
	MY_STACK::iterator Index;
	CBoundary *Bndr;

	Pnt->SetNoise(false);
	
	Pnt->EmptyClust();
	Pnt->AddClust(0);

	//if (Pnt->GetNoise())
	{
	//	Pnt->AddClust(255);
	}
	//else
	{
		vector<bool> vNoise(m_BndrStack.size()); int iB = 0;
		for (Index=m_BndrStack.begin();Index!=m_BndrStack.end();Index++,iB++)
		{
			Bndr = (CBoundary*) *Index;
			Bndr->Clustering(Pnt);
			vNoise[iB] = Bndr->m_bNoise;
		}
		Pnt->SelectCluster(m_ClMode,vNoise);
	}
}

//////////////////////////////////////////////////////////////////////
void CCluster::FindNoise(CVertex *pnt)
{
	// TODO polygons determining user noise

	

}

//////////////////////////////////////////////////////////////////////
void CCluster::FindNoise(CVerxStack *pnts)
{
	MY_STACK::iterator iVerx;

	CVertex *verx;
	for (iVerx = pnts->m_VerxStack.begin(); iVerx != pnts->m_VerxStack.end(); iVerx++)
	{
		verx = (CVertex*) *iVerx;
		FindNoise(verx);
	}
}

//////////////////////////////////////////////////////////////////////
void CCluster::CreateHeader(FILE *file,CString& strInfo)
{
	fprintf(file,"%%%%BEGIN_HEADER CLUSTER_FILE Version 1.4\n");
	fprintf(file,"\n");
	CString strV = GetWClustVersionString();
	fprintf(file,"%%%%WClust version %s\n\n",strV);
	if(strInfo.GetLength()) fprintf(file,"%s",strInfo.GetString());
	fprintf(file,"%%%%BEGIN RECORD_FORMAT_INFORMATION\n");
	fprintf(file,"%%VersionIdentifier ('S')\n");
	fprintf(file,"%%VersionFormat.2 Identifier.1 VersionTimes10.1\n");
	fprintf(file,"%%ExternalParamAmountIdentifier.1 ('W')\n");
	fprintf(file,"%%ExternalParamAmountFormat.2 Identifier.1 Amount.1\n");
	fprintf(file,"%%ExternalParameterIdentifier.1 ('E')\n");
	fprintf(file,"%%ExternalWcpParameterFormat.x Identifier.1 Size.1 ParamName.Size\n");
	fprintf(file,"%%ExtendedDimensionIdentifier.1 ('D')\n");
	fprintf(file,"%%ExtendedDimensionFormat.2 Identifier.1 Size.1\n");
	fprintf(file,"%%ExtendedParameterIdentifier.1 ('R')\n");
	fprintf(file,"%%ExtendedParameterFormat.21 Identifier.1 Type.1 Parameter1.4 Parameter2.4 Parameter3.4 Parameter4.4\n");
	fprintf(file,"%%NoClusterIdentifier.1 ('C')\n");
	fprintf(file,"%%NoClusterFormat.5 Identifier.1 No.4\n");
	fprintf(file,"%%BoundaryIdentifier.1 ('B')\n");
	fprintf(file,"%%BoundaryFormat.5 Identifier.1 NoProjections.4\n");
	fprintf(file,"%%ProjectionIdentifier.1 ('P')\n");
	fprintf(file,"%%ProjectionFormat.13 Identifier.1 AxisX.4 AxisY.4 NumLines.4\n");
	fprintf(file,"%%PointIdentifier.1 ('N')\n");
	fprintf(file,"%%PointFormat.9 Identifier.1 X.4 Y.4\n");
	fprintf(file,"%%VectorIdentifier.1 ('V')\n");
	fprintf(file,"%%VectorFormat.9 Identifier.1 X.4 Y.4 b.4\n");
	
	fprintf(file,"%%PCAParamIdentifier.1 ('A')\n");
	fprintf(file,"%%PCAFormat.10 Identifier.1 PCAType.1 NumPCs.4 StartAxis.4\n");
	
	fprintf(file,"%%%%END RECORD_FORMAT_INFORMATION\n\n");
	fprintf(file,"%%%%END_HEADER\t\t");
}

//////////////////////////////////////////////////////////////////////
int CCluster::Store(CString& strPath,CString& strInfo)
{
	FILE* fp = fopen(strPath.GetBuffer(),"w");
	if ( fp == NULL )
		return 2;
	CreateHeader(fp,strInfo);
	
	fclose(fp);

	CFile file;
	if (!file.Open(strPath,CFile::modeWrite))
		return 2;
	else
	{
		StoreData(&file);
		file.Close();
	}
	m_newChanges = 0;
	return 0;
}

//////////////////////////////////////////////////////////////////////
int CCluster::ChooseFileAndStore(CString& strInfo)
{
    static char BASED_CODE szFilter[] = "Cluster Files (*.cl)|*.cl|All Files (*.*)|*.*||";
	CFileDialog dlg(FALSE, "cl", "*.cl",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,szFilter);
	if (dlg.DoModal()==IDOK)
		return Store(dlg.GetPathName(),strInfo);
	return 2;
}

//////////////////////////////////////////////////////////////////////
void CCluster::AutoSave(char *fileAS,CString& strInfo)
{
	if ( m_newChanges & 2 )
	{
		FILE *strFile;
		
		strFile = fopen(fileAS,"w");
		if ( strFile == NULL )
			return;
		CreateHeader(strFile,strInfo);
		
		fclose(strFile);

		CFile file;
		if (!file.Open(fileAS,CFile::modeWrite))
			return;
		else
		{
			StoreData(&file);
			file.Close();
		}
		m_newChanges &= 0xFD;
	}
}

//////////////////////////////////////////////////////////////////////
void CCluster::StoreData(CFile *file)//,CVerxStack* Pnts)
{
	MY_STACK::iterator Index;

	file->Seek(0,CFile::end);
	// Store VERSION 1.4
	char ver=14;
	file->Write("S",1); file->Write(&ver,1);

	//store pca parameters
	if(m_bUsePCA)
	{
		file->Write("A",1);
		char cType = m_bCombinePCA ? 1 : 0;
		file->Write(&cType,1);
		file->Write(&m_iNumPCsToStore,4);
		file->Write(&m_iPCAStartDim,4);
	}
			
	// Store higher dimension
	int Dim = 0;
	for (Index = m_ParamDimStack.begin(); Index != m_ParamDimStack.end(); Index++)
	{
		Dim++;
	}
	if (Dim>0)
	{
		file->Write("W",1);
		char size = usedExternWCP.size();
		file->Write(&size,sizeof(size));
		MY_WCP_FILES_STACK::iterator indWcp;
		for ( indWcp = usedExternWCP.begin(); indWcp != usedExternWCP.end(); indWcp++ )
		{
			CString extParam = (*indWcp).paramName;
			char length = (char) extParam.GetLength();
			file->Write("E",1);
			file->Write(&length, 1);
			file->Write((LPCTSTR) extParam, length);
		}
		
		file->Write("D",1);
		file->Write(&Dim,sizeof(Dim));
		
		CParamDim *ParDim;
		for (Index = m_ParamDimStack.begin(); Index != m_ParamDimStack.end(); Index++)
		{
			ParDim = (CParamDim*) *Index;
			file->Write("R",1);
			file->Write(&ParDim->m_Type,sizeof(ParDim->m_Type));
			file->Write(&ParDim->m_Par1,sizeof(ParDim->m_Par1));
			file->Write(&ParDim->m_Par2,sizeof(ParDim->m_Par2));
			file->Write(&ParDim->m_Par3,sizeof(ParDim->m_Par3));
			file->Write(&ParDim->m_Par4,sizeof(ParDim->m_Par4));
		}
	}
			
	// Store clusters' data
	file->Write("C",1); 
	file->Write(&m_Count,sizeof(m_Count));
	CBoundary *bndr;
	for (Index=m_BndrStack.begin();Index!=m_BndrStack.end();Index++)
	{
		bndr = (CBoundary*) *Index;
		bndr->StoreData(file);
	}
}

//////////////////////////////////////////////////////////////////////
int CCluster::FindDataStartPtr(CFile *file)
{
	int nRead, iBufSz = 65536;
	vector<char> buffer(iBufSz);
	if (!buffer.size())
		return 3;
	else
	{
		//Searching of index of binary data
		nRead=file->Read(&buffer[0],iBufSz);
		buffer[iBufSz-1] = 0;
		
		char pomStr[20];
		sprintf(pomStr,"%%%%END_HEADER");
		char *pdest = strstr(&buffer[0], pomStr);
		if ( pdest == NULL )
		{
			file->Close();
			return 2;
		}
		dataStartPtr = pdest - &buffer[0] + 1 + 13;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
int CCluster::OpenFile(CFile *file, CVerxStack *Pnts, QBatchRec* pQB)
{
	CString cstrClust;
	if(pQB)
		cstrClust = pQB->strCL.c_str();
	else
	{
		static char BASED_CODE szFilter[] = "Cluster Files (*.cl)|*.cl|All Files (*.*)|*.*||";
		CFileDialog dlg(TRUE, "cl", "*.cl",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,szFilter);
		if(dlg.DoModal()!=IDOK) return 1; // CANCEL
		cstrClust=dlg.GetPathName();
	}

	if (!file->Open(cstrClust,CFile::modeRead))
		return 4;  // file is NOT OPENED

	SetEmpty();
	ClearUNDO();
	Pnts->ClearHighDim();
	Pnts->ClearExternalParamData();
	
	int errorNo = FindDataStartPtr(file);
	if ( errorNo !=0 )
		return errorNo;
	else
		file->Seek(dataStartPtr,0);

	char m_Identif;
	
	// Read version
	file->Read(&m_Identif,1);
	if ( m_Identif =='S' )
	{
		char Version;
		file->Read(&Version,1);
		file->Read(&m_Identif,1);
	}

	// Read external parameter info
	if ( m_Identif == 'W' )
	{
		char numWcp;
		file->Read(&numWcp,1);
		for ( char i = 0; i<numWcp; i++)
		{
			sWcpFiles wcp;
			file->Read(&m_Identif,1);
			if ( m_Identif == 'E' ) 
			{
				char length;
				file->Read(&length, 1);
				vector<char> data(length);
				file->Read(&data[0],length);
				for (char j=0; j<length; j++)
					wcp.paramName.Insert(j,data[j]);
				wcp.flag = numWcp;
				usedExternWCP.push_back(wcp);
			}
			else
			{
				file->Close();
				SetEmpty();
				Pnts->ClearHighDim();
				return 2;
			}
		}
	}
	return 0; // file OPENED
}

//////////////////////////////////////////////////////////////////////
int CCluster::LoadData(CFile *file, CVerxStack *Pnts)
{
	// Set file pointer to the first data identifier
	file->Seek(dataStartPtr,0);
				
	// Main reading of data
	int Count;
	char cIdentif; 
	char Version = 10;
				
	file->Read(&cIdentif,1);
	if ( cIdentif =='S' )
	{
		file->Read(&Version,1);
		file->Read(&cIdentif,1);
	}
	
	if ( Version >= 14)
	{
		if(cIdentif == 'A')
		{
			m_bUsePCA = true;
			char cType;
			file->Read(&cType,1);
			m_bCombinePCA = cType == 1 ? true : false;
			file->Read(&m_iNumPCsToStore,4);
			file->Read(&m_iPCAStartDim,4);
			file->Read(&cIdentif,1);
		}
		else
		{
			m_bUsePCA = false;
			m_bCombinePCA = false;
			m_iNumPCsToStore = 0;
			m_iPCAStartDim = -1;
			m_iPCAEndDim = -1;
		}
	}
	
	if ( cIdentif == 'W' )
	{
		char numWcp;
		file->Read(&numWcp,1);
		for ( char i = 0; i<numWcp; i++)
		{
			file->Read(&cIdentif,1);
			if ( cIdentif == 'E' ) 
			{
				char length;
				file->Read(&length, 1);
				vector<char> data(length*2);
				file->Read(&data[0],length);
			}
		}
		file->Read(&cIdentif,1);
	}
	
	if ( cIdentif == 'D' )
	{	// in file are stored parameters of the higher dimensions
		file->Read(&Count,sizeof(Count));
		for (int iI=0; iI<Count; iI++)
		{
			file->Read(&cIdentif,1);
			if (cIdentif == 'R')
			{
				CParamDim* ParDim = new CParamDim();
				file->Read(&ParDim->m_Type,sizeof(ParDim->m_Type));
				file->Read(&ParDim->m_Par1,sizeof(ParDim->m_Par1));
				file->Read(&ParDim->m_Par2,sizeof(ParDim->m_Par2));
				file->Read(&ParDim->m_Par3,sizeof(ParDim->m_Par3));
				file->Read(&ParDim->m_Par4,sizeof(ParDim->m_Par4));
				AddParam(ParDim,Pnts,false);
			}
			else
			{
				file->Close();
				SetEmpty();
				Pnts->ClearHighDim();
				Pnts->ClearExternalParamData();
				return 2;
			}
		}
		file->Read(&cIdentif,1);
	}							
	
	if ( cIdentif == 'C' )
	{	// Clusters (only 1x in file)
		file->Read(&Count,sizeof(Count));
	}
	else 
	{
		SetEmpty();
		Pnts->ClearHighDim();
		file->Close();
		Pnts->ClearExternalParamData();
		return 2;
	}

	int bndr_Numb=0;
	for (int i=0;i<Count;i++)
	{ 
		int NumbLoad=file->Read(&cIdentif,1);
		if (NumbLoad!=1)
		{
			Pnts->ClearHighDim();
			SetEmpty();
			file->Close();
			Pnts->ClearExternalParamData();
			return 2;
		}
		else if (cIdentif=='B')
		{	//Boundary
			CBoundary* bndr = new CBoundary(m_PalStack);
			bndr->m_Numb=bndr_Numb;
			AddBndry(bndr);
			bndr_Numb++;
	
			int errorLoad;
			if (m_Count>0) 
				errorLoad=bndr->LoadData(file,Version,Pnts->Dimension-Pnts->NumTopDims());
			
			if (errorLoad)
			{
				SetEmpty();	
				Pnts->ClearHighDim();
				file->Close();
				Pnts->ClearExternalParamData();
				return errorLoad;
			}
		}
		else
		{
			Pnts->ClearHighDim();
			SetEmpty();
			file->Close();
			Pnts->ClearExternalParamData();
			return (int) cIdentif;
		}
	}
	file->Close();
	m_newChanges = 0;

	return 0;
}

// For only one spike (which will be stored)
void CCluster::CalcParamOneSpike(CVerxStack *Pnts, CVertex *Pnt, sSpike *spike)
{
	MY_STACK::iterator Index, IndParD;
	
//	Pnts->forStoreBPF_NonClVx->ClearHighDim(Pnts->BASE_DIMENSION);
	Pnt->ClearHighDim(Pnts->BASE_DIMENSION);
	
	for (IndParD=m_ParamDimStack.begin();IndParD!=m_ParamDimStack.end();IndParD++)
	{
		CParamDim* param = (CParamDim*) *IndParD;
		Pnt->CalcParam(param,&Pnts->m_x[0],Pnts->SAMPLE_FREQ,Pnts->NUM_SAMPLES, spike);
	}
}

//calc ALL extra params
void CCluster::CalcParam(CVerxStack *Pnts)
{
	if(!m_ParamDimStack.size())
		return;
	
	Pnts->ClearHighDim();

	TopDimPopPush PopPush(Pnts);

	int iExtraDim = 1;
	
	MY_STACK::iterator IndParD=m_ParamDimStack.begin();
	while (IndParD<m_ParamDimStack.end())
	{
		CParamDim *param = (CParamDim*) *IndParD;
		param->m_iVDim = Pnts->Dimension+iExtraDim;//make sure dimension knows its location so user can remove if wants to
		if( (param->m_Type == 6 && param->m_Par3 == 0) || param->m_Type!=6)
		{
			AddAxes(Pnts,param);
			MY_SPIKE_STACK::iterator indSpikes;
			for ( indSpikes = Pnts->m_SpikesStack.begin(); indSpikes != Pnts->m_SpikesStack.end(); indSpikes++ )
			{
				sSpike& spike = *indSpikes;
				if ( spike.type == 1)
				{
					CVertex* verx = (CVertex*) *( Pnts->m_VerxStack.begin() + spike.offset );
					verx->CalcParam(param,&Pnts->m_x[0],Pnts->SAMPLE_FREQ,Pnts->NUM_SAMPLES, &spike);
				}
			}
		}
		if(param->m_Type == 6)
		{
			IndParD += param->m_Par2;
			iExtraDim += param->m_Par2;
		}
		else
		{
			IndParD++;
			iExtraDim++;
		}
	}
	Pnts->Dimension += m_ParamDimStack.size(); // ???????????????????????????
//	Pnts->BASE_DIMENSION += m_ParamDimStack.size(); // ??????????????????????

//	m_ParamDimStack.clear();

#ifdef _DEBUG
	CString str;
	str.Format("CCluster::CalcParam Dimension=%d BASE_DIMENSION=%d m_ParamDimStack.size()=%d",
		Pnts->Dimension,Pnts->BASE_DIMENSION,m_ParamDimStack.size());
	MessageBox(0,str.GetBuffer(),str.GetBuffer(),0);
#endif
}

//////////////////////////////////////////////////////////////////////
void CCluster::CalcParamLast(CVerxStack *Pnts)
{
	MY_SPIKE_STACK::iterator indSpikes;
	MY_STACK::iterator IndParD;
	
	IndParD = m_ParamDimStack.end()-1;
	CParamDim* param = (CParamDim*) *IndParD;
	for ( indSpikes = Pnts->m_SpikesStack.begin(); indSpikes != Pnts->m_SpikesStack.end(); indSpikes++ )
	{
		sSpike& spike = *indSpikes;
		if ( spike.type == 1)
		{
			CVertex* verx = (CVertex*) *( Pnts->m_VerxStack.begin() + spike.offset );
			verx->CalcParam(param,&Pnts->m_x[0],Pnts->SAMPLE_FREQ,Pnts->NUM_SAMPLES, &spike);
		}
	}
//	Pnts->CalcMinMaxLast();
}

//////////////////////////////////////////////////////////////////////
char CCluster::IsUNDO()
{ 
	if ( undoBndry != NULL )
		return 1; // UNDO can be used
	else
		return 0; // UNDO is empty
}

//////////////////////////////////////////////////////////////////////
void CCluster::ClearUNDO()
{
	if ( IsUNDO() )
	{
		undoType = 0;
		undoBndry->SetEmpty();
		delete undoBndry;
		undoBndry = NULL;
	}
}

/////////////////////////////////////////////////////
// MakeUNDO - type: 0-none, 1-change, 2-insert (after removing)
void CCluster::MakeUNDO(int cluster, char type)
{
	// create data for UNDO parameters
	ClearUNDO();

	MY_STACK::iterator IndBndr, IndProj, IndPnt2D, IndVect2D;

	CBoundary *mainBndr;
	CProjection *newProj;
	CPoint2D *newPnt2D;
	CVect2D *newVect2D;

	if ( cluster!=0 )
	{
		undoType = type;
		IndBndr = (m_BndrStack.begin() + abs(cluster));
		mainBndr = (CBoundary*) *IndBndr;

		undoBndry = new CBoundary(m_PalStack);
		undoBndry->m_Numb = mainBndr->m_Numb;
		undoBndry->m_bNoise = mainBndr->m_bNoise;

		for (IndProj=mainBndr->m_ProjStack.begin();IndProj!=mainBndr->m_ProjStack.end();IndProj++)
		{

			newProj = new CProjection(m_PalStack);
			newProj->m_AxesX = ((CProjection*) *IndProj)->m_AxesX;
			newProj->m_AxesY = ((CProjection*) *IndProj)->m_AxesY;
			undoBndry->AddProj(newProj);

			for (IndPnt2D=((CProjection*)*IndProj)->m_Pnt2DStack.begin();IndPnt2D!=((CProjection*)*IndProj)->m_Pnt2DStack.end();IndPnt2D++)
			{
				newPnt2D = new CPoint2D();
				newPnt2D->m_X = ((CPoint2D*) *IndPnt2D)->m_X;
				newPnt2D->m_Y = ((CPoint2D*) *IndPnt2D)->m_Y;
				newProj->AddPnt2D(newPnt2D);
			}
			
			for (IndVect2D=((CProjection*)*IndProj)->m_Vect2DStack.begin();IndVect2D!=((CProjection*)*IndProj)->m_Vect2DStack.end();IndVect2D++)
			{
				newVect2D = new CVect2D();
				newVect2D->m_X = ((CVect2D*) *IndVect2D)->m_X;
				newVect2D->m_Y = ((CVect2D*) *IndVect2D)->m_Y;
				newVect2D->m_b = ((CVect2D*) *IndVect2D)->m_b;
				newProj->AddVect2D(newVect2D);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
void CCluster::UNDO()
{
	MY_STACK::iterator index;
	CBoundary *bndry;
	
	if ( undoType == 1 )
	{
		// change - after DRAW, REMOVE POLY
		int position = 0;
		index = m_BndrStack.begin();
		bndry = (CBoundary*) *index;
		while ( undoBndry->m_Numb != bndry->m_Numb && index != m_BndrStack.end() )
		{
			index++;
			bndry = (CBoundary*) *index;
			position++;
		}

		if ( index != m_BndrStack.end() )
		{
			bndry->SetEmpty();
			m_BndrStack.erase(index);
			delete bndry;

			m_BndrStack.insert(m_BndrStack.begin()+position,undoBndry);
			
			undoBndry = NULL;
			undoType = 0;
		}
	}

	if ( undoType == 2 )
	{
		// insert - after REMOVE ALL
		index = m_BndrStack.begin() + abs(undoBndry->m_Numb);

		m_BndrStack.insert(index, undoBndry);

		undoBndry = NULL;
		undoType = 0;

		int newI=0;
		for (index = m_BndrStack.begin(); index != m_BndrStack.end(); index++)
		{
			bndry = (CBoundary*) *index;
			bndry->m_Numb = newI;
			newI++;
		}
		m_Count++;
	}
	if ( undoType == 3 )
	{
		// remove - after CREATE NEW, COPY

		MY_STACK::iterator Index,IndProj;
	
		int Selected;
		int i;
		int nalezen=0,counter=0;
		CBoundary *MyBoundary;

		Selected = abs(undoBndry->m_Numb) + 1;
		Selected = (Selected == 0) ? 255 : (Selected - 1);

		if (Selected>0 && Selected < 255)
		{
			CBoundary *Bndr =(CBoundary*) *(m_BndrStack.begin() + Selected);
			m_BndrStack.erase(m_BndrStack.begin()+Selected);
			m_Count -= 1;
			delete Bndr;
			i = Selected;
			for ( Index = (m_BndrStack.begin() + Selected); Index != m_BndrStack.end(); Index++ )
			{
				MyBoundary = (CBoundary*)*Index;
				MyBoundary->m_Numb=i;
				i++;
				Selected++;
			}
			ClearUNDO();
		}
	}

	m_newChanges = 3;
}

int CCluster::GetNumClusts(CVerxStack& DataStack)
{
	switch(DataStack.whichDraw)
	{
	case CLUST_USER:
		return GetCount() - 1;
		break;
	case CLUST_ORIG:
		return DataStack.m_NumOriginalCl;
		break;
	case CLUST_KM:
	case CLUST_INFO:
	case CLUST_AP:
	case CLUST_KK:
	default:
		return m_pNumClusts[DataStack.whichDraw];
		break;
	}
	return 0;
}

struct CIParams
{
	CVerxStack* pDataStack;
	CCluster* pCluster;
	bool bSymmetric;
	A2D<Neighbor> vnn;
	vector<float> vFloat; // 1D array of values -- vector_i dimension_j == vFloat[NumCols*i+j]
	KDTreeHist oTree;
	vector<float> vRange; // range of a dimension, size == dimensions+1
};

prob_t gstartrad = 0.1;

bool ClustIsolationDist(CCluster& Clusters,CVerxStack& oS,vector<float>& vFloat,vector<int>& vClustIDs,vector<int>& vClustCounts,int iClusts,int iRows,int iCols,int WhichDraw,const CUPDUPDATA* pUp)
{	try{
	char msg[1024];
	sprintf(msg,"Computing isolation distance for clusters");
	pUp->SetProgress(msg);
	Write2Log("Computing isolation distance for clusters");
	int iC = 0;
	if(Clusters.m_vInfo[WhichDraw].size() < iClusts+1)
		Clusters.m_vInfo[WhichDraw].resize(iClusts+1);
	vector<float> vRange(iCols),vMin(iCols);
	if(false && Clusters.m_bFindBestDims)
		for(int iD=0;iD<iCols;iD++){
			vRange[iD]=oS.GetMax(iD+1)-oS.GetMin(iD+1);
			vMin[iD]=oS.GetMin(iD+1);
		}
	for(iC=1;iC<=iClusts && !pUp->ShouldTerminate();iC++)
	{	sprintf(msg,"Computing isolation distance for cluster %d",iC);	
		Write2Log(msg);
		pUp->SetProgress(msg,((double)iC/iClusts)*100.0);
		if(vClustCounts[iC]<2)
		{	Write2Log("Clust too small for isolation distance : %d %d , skipping",iC,vClustCounts[iC]);
			continue;
		}
		float fDist = 0.0;
		try
		{	
			if(false && Clusters.m_bFindBestDims)
			{
				vector<float> vFloatICED(iRows*Clusters.m_iBestDims);
				int idx=0;
				for(int iV=0;iV<iRows;iV++)
				{	for(int iD=0;iD<Clusters.m_iBestDims;iD++)
					{
						vFloatICED[idx++]=vRange[Clusters.m_vBestDims[iC][iD]]*vFloat[iV*iCols+Clusters.m_vBestDims[iC][iD]]+
											vMin[Clusters.m_vBestDims[iC][iD]];
					}
				}
				fDist=IsolationDist(vClustIDs,iC,vFloatICED,iRows,Clusters.m_iBestDims);
			}
			else
				fDist=IsolationDist(vClustIDs,iC,vFloat,iRows,iCols);
		}
		catch(...)
		{	Write2Log("Exception computing isolation dist on clust=%d sz=%d",iC,vClustCounts[iC]);
		}
		Clusters.m_vInfo[WhichDraw][iC].m_dIsolationDist = fDist;
		Write2Log("Isolation dist of clust %d is %.6f",iC,fDist);
	}
	return true;
	}catch(...){
		Write2Log("Outer exception in ClustIsolationDist!!");
		return false;
	}
}

bool ClustLRatio(CCluster& Clusters,CVerxStack& oS,vector<float>& vFloat,vector<int>& vClustIDs,vector<int>& vClustCounts,int iClusts,int iRows,int iCols,int WhichDraw,const CUPDUPDATA* pUp)
{	try{
	char msg[1024];
	sprintf(msg,"Computing L-Ratio for clusters");
	pUp->SetProgress(msg);
	Write2Log("Computing L-Ratio for clusters");
	int iC = 0;
	if(Clusters.m_vInfo[WhichDraw].size() < iClusts+1)
		Clusters.m_vInfo[WhichDraw].resize(iClusts+1);
	vector<float> vRange(iCols),vMin(iCols);
	for(iC=1;iC<=iClusts && !pUp->ShouldTerminate();iC++)
	{	sprintf(msg,"Computing L-Ratio for cluster %d",iC);	
		Write2Log(msg);
		pUp->SetProgress(msg,((double)iC/iClusts)*100.0);
		float fLRatio = -1.0;
		try
		{	fLRatio=LRatio(vClustIDs,iC,vFloat,iRows,iCols);
		}
		catch(...)
		{	Write2Log("Exception computing L-Ratio on clust=%d sz=%d",iC,vClustCounts[iC]);
		}
		Clusters.m_vInfo[WhichDraw][iC].m_dLRatio = fLRatio;
		Write2Log("L-Ratio of clust %d is %.6f",iC,fLRatio);
	}
	return true;
	}catch(...){
		Write2Log("Outer exception in ClustLRatio!!");
		return false;
	}
}

/*struct histd
{
	histd(){ n=0; clear(); }
	virtual void clear(){};
	virtual ~histd()=0;
	double n;
};*/

struct hist1
{
	int h[10]; double n;
	hist1(){ n=0.0; clear(); }
	void clear()
	{	int i = 0;
		for(;i<10;i++) h[i]=0;
	}
};

struct hist2
{
	int h[10][10]; double n;
	hist2(){ n=0.0; clear(); }
	void clear()
	{
		int i = 0,j = 0;
		for(i=0;i<10;i++)
			for(j=0;j<10;j++)
				h[i][j]=0;
	}
};

struct hist3
{
	int h[10][10][10]; double n;
	hist3(){ n=0.0; clear(); }
	void clear()
	{
		int i = 0,j = 0,k = 0;
		for(i=0;i<10;i++)
			for(j=0;j<10;j++)
				for(k=0;k<10;k++)
					h[i][j][k]=0;
	}
};

struct hist4
{
	int h[10][10][10][10]; double n;
	hist4(){ n=0.0; clear(); }
	void clear()
	{
		int i = 0,j = 0,k=0,l=0;
		for(i=0;i<10;i++)
			for(j=0;j<10;j++)
				for(k=0;k<10;k++)
					for(l=0;l<10;l++)
						h[i][j][k][l]=0;
	}
};

struct hist5
{
	int h[10][10][10][10][10]; double n;
	hist5(){ n=0.0; clear(); }
	void clear()
	{
		int i = 0,j = 0,k=0,l=0,m=0;
		for(i=0;i<10;i++)
			for(j=0;j<10;j++)
				for(k=0;k<10;k++)
					for(l=0;l<10;l++)
						for(m=0;m<10;m++)
							h[i][j][k][l][m]=0;
	}
};

struct hist6
{
	int h[10][10][10][10][10][10]; double n;
	hist6(){ n=0.0; clear(); }
	void clear()
	{
		int i = 0,j = 0,k=0,l=0,m=0,n=0;
		for(i=0;i<10;i++)
			for(j=0;j<10;j++)
				for(k=0;k<10;k++)
					for(l=0;l<10;l++)
						for(m=0;m<10;m++)
							for(n=0;n<10;n++)
								h[i][j][k][l][m][n]=0;
	}
};

struct hist7
{
	int h[10][10][10][10][10][10][10]; double n;
	hist7(){ n=0.0; clear(); }
	void clear()
	{
		int i = 0,j = 0,k=0,l=0,m=0,n=0,o=0;
		for(i=0;i<10;i++)
			for(j=0;j<10;j++)
				for(k=0;k<10;k++)
					for(l=0;l<10;l++)
						for(m=0;m<10;m++)
							for(n=0;n<10;n++)
								for(o=0;o<10;o++)
									h[i][j][k][l][m][n][o]=0;
	}
};

struct hist8
{
	int h[10][10][10][10][10][10][10][10]; double n;
	hist8(){ n=0.0; clear(); }
	void clear()
	{
		int i = 0,j = 0,k=0,l=0,m=0,n=0,o=0,p=0;
		for(i=0;i<10;i++)
			for(j=0;j<10;j++)
				for(k=0;k<10;k++)
					for(l=0;l<10;l++)
						for(m=0;m<10;m++)
							for(n=0;n<10;n++)
								for(o=0;o<10;o++)
									for(p=0;p<10;p++)
										h[i][j][k][l][m][n][o][p]=0;
	}
};

bool SaveClustProbs(CVerxStack& oS,CCluster& oC,int iDims2Use)
{
	Write2Log("before str dec.");
	CString str;
	Write2Log("after str dec.");

	oS.CalcMinMax();
	
	int iClusts = oC.GetNumClusts(oS) , iC=1;
	int iDims=    oS.GetAutoClusteringDimension(), iD=0;

	int iWhich = oS.whichDraw;

	vector<int> vCounts(iClusts+1);
	oS.GetCounts(vCounts,iClusts, iWhich);
	
	//# of non-noise vertices
	int iNNVertices = oS.NumNonNoiseVertices();

	//for info estimation use Peak-V, even though it is not used in auto-clustering
	vector<int> vZeroes(iDims);

	//multidimensional distributions - continuous
	//resistor average of all points in each cluster against everything OTHER than that cluster points
	vector<float> vFloat,vRange;
	int iRows=0,iCols=0,i=0,j=0;
	oS.GetFloatV(iRows,iCols,vFloat,vRange,false);

	vector<int> vClustIDs;
	oS.GetClustIDs(vClustIDs,oS.whichDraw);

	vector< KDTreeHist > vDistribs, vCompDistribs;

	A2D<int> vBestDims(iClusts+1,iDims2Use);

	if(iDims2Use == 1)
	{	vBestDims.Fill(0);
	}
	else if(iDims2Use <= 4)
	{	for(i=1;i<=iClusts;i++)
			for(j=0;j<iDims2Use;j++)
				vBestDims[i][j]=j;
	}
	else if(iDims2Use <= 8)
	{	for(i=1;i<=iClusts;i++)
		{	for(j=0;j<4;j++)
				vBestDims[i][j]=j;
			int dd=4;
			for(j=16;j<20 && dd<iDims2Use;j++,dd++)
				vBestDims[i][dd]=j;
		}
	}

	FillDistribs(vFloat,vDistribs,vCompDistribs,iClusts+1,vClustIDs,vCounts,iCols,vBestDims,iDims2Use);

	A2D<char> vStrs(iRows,iDims2Use+1);
	HashHist BG;
	double maxbin = 9.0;
	int r;
	for(r=0;r<iRows;r++)
	{	
		for(b=0;b<iDims2Use;b++)
			vStrs[r][b] = char(vFloat[iCols*r+vBestDims[1][b]] * maxbin + 0.5);
		vStrs[r][b]=0;
		BG.AddVertex(vStrs[r]);
	}
	int c;
	for(c=1;c<=iClusts;c++)
	{	str.Format(".\\tmp\\__kdtree_probs_clust%d_dims%d.txt",c,iDims2Use);
		FILE* fp = fopen(str.GetBuffer(),"w"); if(!fp) return false;
		str.Format(".\\tmp\\__hist_probs_clust%d_dims%d.txt",c,iDims2Use);
		FILE* fpb = fopen(str.GetBuffer(),"w"); if(!fpb) return false;
		KDTreeHist& oT = vDistribs[c];
		int iSz = oT.NumElems();

		HashHist CH;

		if(true || iDims2Use == 1)
		{
			int r = 0 , bin = 0;
			for(r=0;r<iRows;r++)
			{	if(vClustIDs[r]==c)
				{
					CH.AddVertex(vStrs[r]);
				}
			}
			Write2Log("filled %d-D hist for clust %d",iDims2Use,c);
			for(j=0;j<iSz;j++)
			{	fprintf(fp,"%g\n",oT.IProb(j));
				fprintf(fpb,"%g\n",CH.Prob(vStrs[j]));
			}
		}
		else if(iDims2Use == 2)
		{	auto_ptr<hist2> p(new hist2);
			hist2& h2 = *p.get();
			int r = 0;
			for(r=0;r<iRows;r++)	//fill histogram
			{	if(vClustIDs[r]==c)
				{	int bin1 = int(vFloat[iCols*r+vBestDims[c][0]] * 9.0 + 0.5);
					int bin2 = int(vFloat[iCols*r+vBestDims[c][1]] * 9.0 + 0.5);
					h2.h[bin1][bin2]++;
					h2.n++;
				}
			}
			for(j=0;j<iSz;j++) // write probabilities to file
			{	fprintf(fp,"%g\n",oT.IProb(j));
				fprintf(fpb,"%g\n",h2.h[int(9.0 * oT[j][0] + 0.5)][int(9.0 * oT[j][1] + 0.5)] / h2.n);
			}
		}
		else if(iDims2Use == 3)
		{	auto_ptr<hist3> p(new hist3);
			hist3& h3 = *p.get();
			int r = 0;
			for(r=0;r<iRows;r++)	//fill histogram
			{	if(vClustIDs[r]==c)
				{	int bin1 = int(vFloat[iCols*r+vBestDims[c][0]] * 9.0 + 0.5);
					int bin2 = int(vFloat[iCols*r+vBestDims[c][1]] * 9.0 + 0.5);
					int bin3 = int(vFloat[iCols*r+vBestDims[c][2]] * 9.0 + 0.5);
					h3.h[bin1][bin2][bin3]++;
					h3.n++;
				}
			}
			for(j=0;j<iSz;j++) // write probabilities to file
			{	fprintf(fp,"%g\n",oT.IProb(j));
				fprintf(fpb,"%g\n",h3.h[int(9.0 * oT[j][0] + 0.5)][int(9.0 * oT[j][1] + 0.5)][int(9.0 * oT[j][2] + 0.5)] / h3.n);
			}
		}
		else if(iDims2Use == 4)
		{	auto_ptr<hist4> p(new hist4);
			hist4& h4 = *p.get();
			int r = 0, bin[4] , b = 0;
			for(r=0;r<iRows;r++)	//fill histogram
			{	if(vClustIDs[r]==c)
				{	for(b=0;b<4;b++)
						bin[b] = int(vFloat[iCols*r+vBestDims[c][b]] * 9.0 + 0.5);
					h4.h[bin[0]][bin[1]][bin[2]][bin[3]]++;
					h4.n++;
				}
			}
			for(j=0;j<iSz;j++) // write probabilities to file
			{	fprintf(fp,"%g\n",oT.IProb(j));
				fprintf(fpb,"%g\n",h4.h[int(9.0 * oT[j][0] + 0.5)][int(9.0 * oT[j][1] + 0.5)][int(9.0 * oT[j][2] + 0.5)][int(9.0 * oT[j][3] + 0.5)] / h4.n);
			}
		}
		else if(iDims2Use == 5)
		{	auto_ptr<hist5> p(new hist5);
			hist5& h5 = *p.get();
			int r = 0, bin[5] , b = 0;
			for(r=0;r<iRows;r++)	//fill histogram
			{	if(vClustIDs[r]==c)
				{	for(b=0;b<5;b++)
						bin[b] = int(vFloat[iCols*r+vBestDims[c][b]] * 9.0 + 0.5);
					h5.h[bin[0]][bin[1]][bin[2]][bin[3]][bin[4]]++;
					h5.n++;
				}
			}
			for(j=0;j<iSz;j++) // write probabilities to file
			{	fprintf(fp,"%g\n",oT.IProb(j));
				fprintf(fpb,"%g\n",h5.h[int(9.0*oT[j][0] + 0.5)][int(9.0*oT[j][1] + 0.5)][int(9.0*oT[j][2] + 0.5)][int(9.0*oT[j][3] + 0.5)][int(9.0*oT[j][4] + 0.5)]/h5.n);
			}
			Write2Log("wrote all to file");
		}
		else if(iDims2Use == 6)
		{	auto_ptr<hist6> p(new hist6);
			hist6& h6 = *p.get();
			int r = 0, bin[6] , b = 0;
			for(r=0;r<iRows;r++)	//fill histogram
			{	if(vClustIDs[r]==c)
				{	for(b=0;b<6;b++)
						bin[b] = int(vFloat[iCols*r+vBestDims[c][b]] * 9.0 + 0.5);
					h6.h[bin[0]][bin[1]][bin[2]][bin[3]][bin[4]][bin[5]]++;
					h6.n++;
				}
			}
			for(j=0;j<iSz;j++) // write probabilities to file
			{	fprintf(fp,"%g\n",oT.IProb(j));
				fprintf(fpb,"%g\n",h6.h[int(9.0 * oT[j][0] + 0.5)][int(9.0 * oT[j][1] + 0.5)][int(9.0 * oT[j][2] + 0.5)][int(9.0 * oT[j][3] + 0.5)][int(9.0 * oT[j][4] + 0.5)][int(9.0 * oT[j][5] + 0.5)] / h6.n);
			}
		}
		else if(iDims2Use == 7)
		{	auto_ptr<hist7> p(new hist7);
			hist7& h7 = *p.get();
			int r = 0, bin[7] , b = 0;
			for(r=0;r<iRows;r++)	//fill histogram
			{	if(vClustIDs[r]==c)
				{	for(b=0;b<7;b++)
						bin[b] = int(vFloat[iCols*r+vBestDims[c][b]] * 9.0 + 0.5);
					h7.h[bin[0]][bin[1]][bin[2]][bin[3]][bin[4]][bin[5]][bin[6]]++;
					h7.n++;
				}
			}
			for(j=0;j<iSz;j++) // write probabilities to file
			{	fprintf(fp,"%g\n",oT.IProb(j));
				fprintf(fpb,"%g\n",h7.h[int(9.0 * oT[j][0] + 0.5)][int(9.0 * oT[j][1] + 0.5)][int(9.0 * oT[j][2] + 0.5)][int(9.0 * oT[j][3] + 0.5)][int(9.0 * oT[j][4] + 0.5)][int(9.0 * oT[j][5] + 0.5)][int(9.0 * oT[j][6] + 0.5)] / h7.n);
			}
		}
		else if(iDims2Use == 8)
		{	auto_ptr<hist8> p(new hist8);
			hist8& h8 = *p.get();
			int r = 0, bin[8] , b = 0;
			for(r=0;r<iRows;r++)	//fill histogram
			{	if(vClustIDs[r]==c)
				{	for(b=0;b<8;b++)
						bin[b] = int(vFloat[iCols*r+vBestDims[c][b]] * 9.0 + 0.5);
					h8.h[bin[0]][bin[1]][bin[2]][bin[3]][bin[4]][bin[5]][bin[6]][bin[7]]++;
					h8.n++;
				}
			}
			for(j=0;j<iSz;j++) // write probabilities to file
			{	fprintf(fp,"%g\n",oT.IProb(j));
				fprintf(fpb,"%g\n",h8.h[int(9.0*oT[j][0] + 0.5)][int(9.0*oT[j][1] + 0.5)][int(9.0*oT[j][2] + 0.5)][int(9.0*oT[j][3] + 0.5)][int(9.0*oT[j][4] + 0.5)][int(9.0*oT[j][5] + 0.5)][int(9.0*oT[j][6] + 0.5)][int(9.0*oT[j][7] + 0.5)]/h8.n);
			}
		}
		fclose(fp); fclose(fpb);
		Write2Log("closed files");
	}
	delete [] tmpstr;
	Write2Log("returning true");
	return true;
}

bool CIThread(const CUPDUPDATA* pUp)
{	extern int iSQRTCalls; iSQRTCalls = 0;
	try
	{
		CIParams* pParams = (CIParams*) pUp->GetAppData();

		CVerxStack& DataStack = *pParams->pDataStack;
		CCluster& Clusters = *pParams->pCluster;
		bool bSymmetric = pParams->bSymmetric;
		A2D<Neighbor>& vnn = pParams->vnn;
		vector<float>& vFloat = pParams->vFloat;
		KDTreeHist& oTree = pParams->oTree;//full tree
		vector<float>& vRange = pParams->vRange;

		if(false)
		{	pUp->SetProgress("saving clust. probs dim=1"); SaveClustProbs(DataStack,Clusters,1);
			pUp->SetProgress("saving clust. probs dim=2"); SaveClustProbs(DataStack,Clusters,2);
			pUp->SetProgress("saving clust. probs dim=3"); SaveClustProbs(DataStack,Clusters,3);
			pUp->SetProgress("saving clust. probs dim=4"); SaveClustProbs(DataStack,Clusters,4);
			pUp->SetProgress("saving clust. probs dim=5"); SaveClustProbs(DataStack,Clusters,5);
			pUp->SetProgress("saving clust. probs dim=6"); SaveClustProbs(DataStack,Clusters,6);
			pUp->SetProgress("saving clust. probs dim=7"); SaveClustProbs(DataStack,Clusters,7);
			pUp->SetProgress("saving clust. probs dim=8"); SaveClustProbs(DataStack,Clusters,8);
			return true;
		}

		Write2Log("CIThread for BPF = %s",DataStack.m_FileName);		

		char msg[1024]={0};

		DataStack.CalcMinMax();
		
		int iClusts = Clusters.GetNumClusts(DataStack) , iC=1;
		int iDims=    DataStack.GetAutoClusteringDimension(), iD=0;

		int iWhich = DataStack.whichDraw;

		vector<int> vCounts(iClusts+1);
		DataStack.GetCounts(vCounts,iClusts, iWhich);
		
		//# of non-noise vertices
		int iNNVertices = DataStack.NumNonNoiseVertices();

		//for info estimation use Peak-V, even though it is not used in auto-clustering
		vector<int> vZeroes(iDims);
		
		if(Clusters.m_vInfo[iWhich].size()<iClusts+1)
			Clusters.m_vInfo[iWhich] = vector< ClusterInfo >(iClusts+1);
		for(iC=1;iC<=iClusts;iC++)
		{	Clusters.m_vInfo[iWhich][iC].m_iMyID = iC;
			Clusters.m_vInfo[iWhich][iC].m_iSz = vCounts[iC];
		}

		//multidimensional distributions - continuous
		//resistor average of all points in each cluster against everything OTHER than that cluster points
		int iRows=0,iCols=0;
		DataStack.GetFloatV(iRows,iCols,vFloat,vRange,false);

		vector<int> vClustIDs;
		DataStack.GetClustIDs(vClustIDs,DataStack.whichDraw);

		vector< KDTreeHist > vDistribs, vCompDistribs;

		const bool bFindBestDims = Clusters.m_bFindBestDims;
		A2D<int>& vBestDims = Clusters.m_vBestDims;
		A2D<prob_t>& v1DKLDivs = Clusters.m_v1DKLDivs;
		A2D<KLD2D>& v2DKLDivs = Clusters.m_v2DKLDivs;
		const int iBestDims = Clusters.m_iBestDims;
		const bool b1D = Clusters.m_b1DBest;
		if(bFindBestDims)
		{	sprintf(msg,"Finding best %d dimensions for each of the %d clusters.",iBestDims,iClusts);
			pUp->SetProgress(msg); Write2Log(msg);
			Write2Log("Turning off fast mode"); Clusters.m_bFastKLD = false;
			if(b1D)
			{	//find best dims using 1D slices
				FindBest1DDims(vFloat,iClusts,iCols,iBestDims,vCounts,vClustIDs,vBestDims,v1DKLDivs,
							   DataStack.m_AxesStack,pUp);
				//printout for debuggering
				Write2Log("v1DKLDivs:\n");	WriteMat2Log(v1DKLDivs);
				Write2Log("\nvBestDims");	WriteMat2Log(vBestDims);
				int x=0,y=0;
				MY_STR_STACK& vAxes = DataStack.m_AxesStack;
				for(y=1;y<vBestDims.Rows();y++)
				{	Write2Log("\nclust%d best dims:\t",y);
					for(x=0;x<vBestDims.Cols();x++) Write2Log("%s index=%d\t",*vAxes[vBestDims[y][x]],vBestDims[y][x]);
					Write2Log("\n\n");
				}
			}
			else	//find best dims using 2D slices
				FindBest2DDims(vFloat,vRange,iClusts,iCols,iBestDims,vCounts,vClustIDs,vBestDims,v2DKLDivs,
							   DataStack.m_AxesStack,pUp);
			FillDistribs(vFloat,vDistribs,vCompDistribs,iClusts+1,vClustIDs,vCounts,iCols,vBestDims,iBestDims);
			Write2Log("filled distribs");
		}
		else
			FillDistribs(vFloat,vDistribs,vCompDistribs,iClusts+1,vClustIDs,vCounts,iCols,true);

		const bool bDoSilhouette = Clusters.m_bSilhouette;
		if(bDoSilhouette && bFindBestDims)
		{	Write2Log("Computing Silhouette Width for each cluster");
			Silhouette(vFloat,iCols,iRows,vClustIDs,vCounts,iClusts,Clusters.m_vInfo[iWhich],vBestDims,iBestDims,pUp);
		}

		bool bFast = Clusters.m_bFastKLD && (Clusters.m_bDoKLDiv || Clusters.m_bInterKLD);
		int iNNToFind = 1; 
		
		if(Clusters.m_bInterKLD || Clusters.m_bDoKLDiv)Write2Log("Starting CalcClustInfo bFast=%s NumElems=%d NumDims=%d",bFast?"true":"false",iRows,iCols);
		Neighbor** pnn = 0; //neighbors , only used in fast mode
		vector<int> vFound(iRows,1);
		if(bFast)
		{	if(!vnn.Init(iRows,iNNToFind))
			{	Write2Log("CIThread: Out of memory, couldn't alloc %d by %d Neighbors",iRows,iNNToFind);
				return false;
			}
			pnn = vnn.GetP();

			//find 1st nearest neighbors for each vertex
			oTree.SetData(iCols,&vFloat[0],iRows);
			sprintf(msg,"Finding initial %d nearest neighbors...",iNNToFind);
			pUp->SetProgress(msg);
			
			int k;
			for(k=0;k<iRows;k++)
			{	if(k%1000==0)
				{	sprintf(msg,"Finding nearest neighbor for vector %d",k);
					pUp->SetProgress(msg,100*(k/static_cast<double>(iRows)));
				}
				oTree.GetNearestNeighbor(oTree[k],false,pnn[k][0]);
			}
		}

		if(bFast){
		ScopedTimer S("NeighborClustCount");
		NeighborClustCount(pnn,vClustIDs,Clusters.m_vInfo[iWhich],iNNToFind,iClusts);
		int s = 0;
		for(s=1;s<=iClusts;s++)
		{
			Write2Log("clust %d avg prct %d-nearest-neighbors in same clust = %.4f",s,iNNToFind,Clusters.m_vInfo[iWhich][s].m_fPrctKNNInClust);
		}
		}
		
		vector<prob_t> vcInf(iClusts+1);//kldiv resistor avg. cluster vs background
		if(Clusters.m_bDoKLDiv) for(iC=1;iC<=iClusts && !pUp->ShouldTerminate();iC++)
		{
			try
			{	sprintf(msg,"Calculating background kldiv clust %d of %d",iC,iClusts);
				pUp->SetProgress(msg,100*(iC/static_cast<double>(iClusts)));
			}
			catch(...)
			{	Write2Log("Exception in progress control Cluster=%d Count=%d",iC,vCounts[iC]);
			}

			prob_t kldiv=0.0f;
		
			if(vCounts[iC])
			{
				try
				{	if(bFast)
					{	kldiv = FastKLDivSymPNOTP(vDistribs[iC],vCompDistribs[iC],pnn,iC,vClustIDs,iNNToFind,vFound);
					}
					else
					{
						kldiv = bSymmetric ? 
							KLDivSym(vDistribs[iC],vCompDistribs[iC]) : KLDiv(vDistribs[iC],vCompDistribs[iC]);
					}
					Write2Log(bSymmetric?"sym. kldiv=%.4f":"kldiv=%.4f",kldiv);
				}
				catch(...)
				{	Write2Log("Exception in KLDivSym Cluster=%d Count=%d",iC,vCounts[iC]);
				}
				if(kldiv<0.0) Write2Log("kldiv<0.0 Cluster=%d Count=%d",iC,vCounts[iC]);
			}
			else
				Write2Log("Empty Cluster!!! Cluster=%d Count=0",iC);
			try
			{	//resistor average of kldiv from cluster iC to background
				Clusters.m_vInfo[DataStack.whichDraw][iC].m_fBGInfoGain = kldiv;
			}
			catch(...)
			{	Write2Log("exception in assigning Clusters.m_vInfo Cluster=%d Count=%d",iC,vCounts[iC]);
			}
		}
		if(Clusters.m_bInterKLD) //kldiv cluster to other clusters
			InterClustKLD(Clusters,vDistribs,vClustIDs,vCounts,iClusts,bFast,pnn,DataStack.whichDraw,pUp,
							iNNToFind,vFound,vFloat,iRows,iCols);

		if(Clusters.m_bDoKLDiv) for(iC=1;iC<=iClusts && !pUp->ShouldTerminate();iC++)
			Write2Log("clust=%d count=%d kldiv_total=%.4f kldiv_bg=%.4f kldiv_inter=%.4f closest_clust=%d",
					  iC,vCounts[iC],
					  Clusters.m_vInfo[DataStack.whichDraw][iC].m_fBGInfoGain+Clusters.m_vInfo[DataStack.whichDraw][iC].m_fInterClustGain,
					  Clusters.m_vInfo[DataStack.whichDraw][iC].m_fBGInfoGain,
					  Clusters.m_vInfo[DataStack.whichDraw][iC].m_fInterClustGain,
					  Clusters.m_vInfo[DataStack.whichDraw][iC].m_iClosestID);

		if((Clusters.m_bDoKLDiv || Clusters.m_bInterKLD))
			Write2Log("Finished CalcClustInfo bFast=%d bDoKLDiv=%d bInter=%d NumElems=%d NumDims=%d iSQRTCalls=%d bFindBestDims=%d iBestDims=%d",
			bFast?1:0,Clusters.m_bDoKLDiv?1:0,Clusters.m_bInterKLD?1:0,iRows,iCols,iSQRTCalls,Clusters.m_bFindBestDims?1:0,Clusters.m_iBestDims);
		
		if(Clusters.m_bIsolationDist || Clusters.m_bLRatio)
		{	pUp->SetProgress("Acquiring energy AND energy-normalized PC1 for each tetrode.");
			DataStack.DoVertexPCA(true);		
		}
		if(Clusters.m_bIsolationDist)	//isolation distance
			ClustIsolationDist(Clusters,DataStack,DataStack.ICED(),vClustIDs,vCounts,iClusts,iRows,DataStack.NUM_CHANNELS*2,DataStack.whichDraw,pUp);
		if(Clusters.m_bLRatio) // L-Ratio 
			ClustLRatio(Clusters,DataStack,DataStack.ICED(),vClustIDs,vCounts,iClusts,iRows,DataStack.NUM_CHANNELS*2,DataStack.whichDraw,pUp);
		if(Clusters.m_bIsolationDist || Clusters.m_bLRatio)
			DataStack.ClearICED();
		return true;
	} 
	catch(...)
	{	Write2Log("CIThread: Outermost Exception!");
		return false;
	}
}


bool CCluster::CalcClusterInfo(CVerxStack& DataStack,bool bSymmetric,HWND wnd)
{
	try
	{
		CIParams oParams;
		oParams.pDataStack = &DataStack;
		oParams.pCluster = this;
		oParams.bSymmetric = bSymmetric;

		std::string str("Computing Cluster Quality");

		//create progress monitor and thread to compute cluster quality
		CUPDialog o(wnd,CIThread,&oParams,str.c_str(),true);
		if(o.DoModal()==IDOK)
			return true;
	
		return false;
	}
	catch(...)
	{
		return false;
	}
}

void CCluster::KmeansClust(CVerxStack& DataStack,int iMinClust,int iMaxClust,int iIter,int iDBIter,bool bNormalize,bool bUseMedians,const CUPDUPDATA* pUp)
{
	int iDims=DataStack.GetAutoClusteringDimension() , iD=0;

	//using cdna library for k-means
	int iRows = 0, iCols = 0;
	double** pData;
	if(bNormalize)
		pData = DataStack.NormalizedPV<double>(true,iRows,iCols);
	else
		pData = DataStack.GetPV<double>(true,iRows,iCols);

	AutoFree2D<double> AFData(pData);

	int** pMask = Allocate2DArray<int>(iRows,iCols);
	AutoFree2D<int> AFMask(pMask);
	int i,j;
	for(i=0;i<iRows;i++)
		for(j=0;j<iCols;j++)
			pMask[i][j]=1;

	vector<double> Weight(iDims,1.0);
	//used to skip Peak-V, Valley-V, Valley
	int k;
	vector<int> vZeroes(iDims);
	for(k=4;k<=7;k++)   Weight[k] = 0.0; // skip Peak-V
	for(k=8;k<=11;k++)  Weight[k] = 0.0; // skip Valley
	for(k=12;k<=15;k++) Weight[k] = 0.0; // skip Valley-V
	k=0;

	//cluster ids
	vector<int> clusterID(iRows);

	int iTranspose = 0;

	//sum of distances from elements in cluster to centroid of cluster
	double sumDist = 0.0;
	//# of time optimal solution found
	int iFound = 0;

	char cMethod = bUseMedians ? 'm' : 'a';// whether to use k-medians
	char cDist = bUseMedians ? 'b' : 'e';//city-block with k-medians, euclidean with k-means

	int iClusts = iMinClust;

	if(iMinClust == iMaxClust)
	{
		kcluster(iClusts,iRows,iCols,pData,pMask,&Weight[0],iTranspose,iIter,cMethod,cDist,&clusterID[0],&sumDist,&iFound,pUp);
	}
	else
	{	//use davies-bouldin index to estimate optimal # of clusters
		pUp->SetProgress("Estimating optimal # of clusters using DB-index",0);
		int iC = iMinClust;
		vector<double> vDBIndex(iMaxClust+1);
		char msg[1024];
		for(iC=iMinClust;iC<=iMaxClust && !pUp->ShouldTerminate();iC++)
		{
			sprintf(msg,"Computing DB-index for %d clusters",iC);
			pUp->SetProgress(msg,99.0*(iC-iMinClust)/(double)(iMaxClust-iMinClust));

			kcluster(iC,iRows,iCols,pData,pMask,&Weight[0],iTranspose,iDBIter,cMethod,cDist,&clusterID[0],&sumDist,&iFound,pUp);
			int iC1,iC2;
			double dbIndex = 0.0;
			int iE = 0;
			vector< vector<int> > vTmpIDs(iC,vector<int>());
			for(iE=0;iE<iRows;iE++) vTmpIDs[clusterID[iE]].push_back(iE);
			for(iC1=0;iC1<iC;iC1++)
			{	vector<double> v(iC); v.resize(0);

				sprintf(msg,"Computing intra-cluster distance for clust %d",iC1+1);
				pUp->SetProgress(msg);
				double d11 = clusterdistance(iRows,iCols,pData,pMask,&Weight[0],vTmpIDs[iC1].size(),vTmpIDs[iC1].size() ,&vTmpIDs[iC1][0] ,&vTmpIDs[iC1][0] ,cDist,'v',iTranspose);

				for(iC2=0;iC2<iC;iC2++)
				{
					if(iC1==iC2) continue;
					
					sprintf(msg,"Computing inter-cluster distance for clust %d and %d",iC1+1,iC2+1);
					pUp->SetProgress(msg);
					
					double d22 = clusterdistance(iRows,iCols,pData,pMask,&Weight[0],vTmpIDs[iC2].size(),vTmpIDs[iC2].size() ,&vTmpIDs[iC2][0] ,&vTmpIDs[iC2][0] ,cDist,'v',iTranspose);
					double d12 = clusterdistance(iRows,iCols,pData,pMask,&Weight[0],vTmpIDs[iC1].size(),vTmpIDs[iC2].size() ,&vTmpIDs[iC1][0] ,&vTmpIDs[iC2][0] ,cDist,'v',iTranspose);
					v.push_back( ( d11 + d22 ) / d12);
				}
				dbIndex += *std::max_element(v.begin(),v.end());
			}
			vDBIndex[iC] = dbIndex / iC;
		}
		double dMinDB = vDBIndex[iMinClust];
		iClusts = iMinClust;
		for(iC=iMinClust+1;iC<=iMaxClust;iC++)
		{
			if(vDBIndex[iC]<dMinDB)
			{
				dMinDB = vDBIndex[iC];
				iClusts=iC;
			}
		}
		sprintf(msg,"using %d clusters for full %d iterations, dbIndex was %f",iClusts,iIter,dMinDB);
		pUp->SetProgress(msg);
		Sleep(3000);//let user read message

		kcluster(iClusts,iRows,iCols,pData,pMask,&Weight[0],iTranspose,iIter,cMethod,cDist,&clusterID[0],&sumDist,&iFound,pUp);
	}

	m_pNumClusts[CLUST_KM] = iClusts;

	MY_STACK::iterator Index;
	int iV = 0;
	for (Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++,iV++)
	{	
		CVertex* verx = (CVertex*)*Index;

		//skip noise
		if(verx->GetNoise()) continue;

		verx->SetKmeansClust(clusterID[iV]+1);
	}

	pUp->SetProgress("Finishing K-means clustering",100);
}

void RemoveVertex(vector< Hist>& vDistrib,CVertex* verx,int iDims)
{
	//move vertex OUT of distribution for each dimension
	int iD = 0;
	for(iD=0;iD<iDims;iD++) vDistrib[iD].DecBinVal(verx->GetValue(iD+1));
}

void AddVertex(vector< Hist >& vDistrib,CVertex* verx,int iDims)
{
	//move vertex INTO distribution for each dimension
	int iD = 0;
	for(iD=0;iD<iDims;iD++) vDistrib[iD].IncBinVal(verx->GetValue(iD+1));
}

void RemoveVertex(vector< Hist>& vDistrib,int* pVBin,int iDims)
{
	//move vertex OUT of distribution for each dimension
	int iD = 0;
	for(iD=0;iD<iDims;iD++) vDistrib[iD].DecBin(pVBin[iD]);
}

void AddVertex(vector< Hist >& vDistrib,int* pVBin,int iDims)
{
	//move vertex INTO distribution for each dimension
	int iD = 0;
	for(iD=0;iD<iDims;iD++) vDistrib[iD].IncBin(pVBin[iD]);
}

void randomassign (int nclusters, int nelements, int clusterid[]);

void CCluster::ExclusiveInfoClust(CVerxStack& DataStack,int iClusts,const CUPDUPDATA* pUp)
{	
	ProbInitFree oPIF(DataStack.m_VerxStack.size()+1);

	const bool bUseCounts = false;

	extern int pBins[4];
	const int iBins = pBins[2]; //initialized to 30, but may be modified by user
	int k = 0;

	int iDims=DataStack.GetAutoClusteringDimension() , iD=0;

	vector< prob_t > vInfo(iClusts+1);

	m_pNumClusts[CLUST_INFO] = iClusts;

	vector<int> vClustIDs(DataStack.NumNonNoiseVertices()) , vCounts(iClusts+1);

	randomassign(iClusts,vClustIDs.size(),&vClustIDs[0]);
	//make it 1-based indexing and init counts
	for(k=0;k<vClustIDs.size();k++)
	{
		vClustIDs[k]++;
		vCounts[vClustIDs[k]]++;
	}

	k=0;

	int iRows = 0, iCols = 0;
	int** pBinData = DataStack.GetVBinIDs<int>(true,iRows,iCols,iBins);
	AutoFree2D<int> AFData(pBinData);

	//fill distributions once
	vector< vector< Hist > > vDistribs;
	FillDistribs(DataStack,pBinData,*this,iBins,vDistribs,iClusts+1,vClustIDs);

	int iV = 0;

	//approximate exclusive information gain
	vector<prob_t> vcInf(iClusts+1);

	//only need this for first time, since its recalculated for distributions who's
	//points change, when they change...
	CalcEXInfo(vDistribs,vcInf,iClusts,iDims,vCounts,iRows);

	int iIter = 0, iPeriod = 10;
	
	vector<int> vSaved(vClustIDs.size());

	k = 0;

	char msg[1024];

	while(true)
	{
		if(pUp && pUp->ShouldTerminate()) break;

		if (iIter % iPeriod == 0) /* Save the current cluster assignments */
		{
			int i;
			for (i = 0; i < vClustIDs.size(); i++) vSaved[i] = vClustIDs[i];
			if (iPeriod < INT_MAX / 2) iPeriod *= 2;
		}

		++iIter;

		bool bMoved = false;

		int iV = 0 , iJnk = 0;

		for(iV=0;iV<vClustIDs.size();iV++,iJnk++)
		{
			if(pUp)
			{
				if(pUp->ShouldTerminate()) break;

				if(iJnk % 25 == 0)
				{
					sprintf(msg,"Iteration %d e-info-max clustering checking vertex %d of %d",
						iIter,iV,vClustIDs.size());
					pUp->SetProgress(msg,99.0 * (prob_t) iV / (prob_t) vClustIDs.size());
				}
			}

			int iOrig = vClustIDs[iV] , iT = 1;

			if(vCounts[iOrig] <= 1) continue; //don't make any empty clusters

			vector<prob_t> vGainSums(iClusts+1);

			//original sum of info gains
			vGainSums[iOrig] = Sum(vcInf);
			
			//move vertex OUT of original distribution
			RemoveVertex(vDistribs[iOrig],pBinData[iV],iDims);			
			prob_t dGainOut = 0.0;			
			//calculate gain for new distrib by first moving vertex out of original distribution
			//it will then be moved into each of the other clusters, and full info gain computed
			for(iD=0;iD<iDims;iD++) dGainOut += KLDivApproxExclusiveProb(vDistribs[iOrig][iD],vDistribs[iClusts+1][iD]);
			if(bUseCounts)
				dGainOut *= (prob_t) (vCounts[iOrig] - 1.0) / (prob_t) iRows;
			AddVertex(vDistribs[iOrig],pBinData[iV],iDims);

			for(iT=1;iT<=iClusts;iT++)
			{
				if(iT == iOrig) continue;

				vector<prob_t> vcInfTmp(vcInf);
				vcInfTmp[iOrig] = dGainOut;

				//move vertex INTO temporary distribution
				AddVertex(vDistribs[iT],pBinData[iV],iDims);

				//calculate NET gain from moving vertex INTO temp distribution
				prob_t dGainIn=0.0;
				for(iD=0;iD<iDims;iD++) dGainIn += KLDivApproxExclusiveProb(vDistribs[iT][iD],vDistribs[iClusts+1][iD]);
				if(bUseCounts)
					dGainIn *= (prob_t) (vCounts[iT] + 1.0) / (prob_t) iRows;
				
				vcInfTmp[iT] = dGainIn;

				vGainSums[iT] = Sum(vcInfTmp);

				//move vertex OUT of temporary distribution
				RemoveVertex(vDistribs[iT],pBinData[iV],iDims);
			}

			//now see if moving the vertex to a different cluster had any net positive information gain
			int iMaxInd = iOrig; prob_t dMaxGain = vGainSums[iOrig];
			for(iT=1;iT<=iClusts;iT++)
			{
				if(iT==iOrig) continue;
				if(vGainSums[iT] > dMaxGain)
				{
					dMaxGain = vGainSums[iT];
					iMaxInd = iT;
				}
			}

			if(iMaxInd != iOrig)
			{
				bMoved = true;

				vClustIDs[iV] = iMaxInd;
				
				//move vertex INTO destination distribution
				AddVertex(vDistribs[iMaxInd],pBinData[iV],iDims);
				vCounts[iMaxInd]++;
				prob_t dGain = 0.0;
				for(iD=0;iD<iDims;iD++) dGain += KLDivApproxExclusiveProb(vDistribs[iMaxInd][iD],vDistribs[iClusts+1][iD]);
				if(bUseCounts)
					vcInf[iMaxInd] = ((prob_t)vCounts[iMaxInd] / (prob_t)iRows) * dGain;
				else
					vcInf[iMaxInd] = dGain;

				//move vertex OUT of original distribution
				RemoveVertex(vDistribs[iOrig],pBinData[iV],iDims);
				vCounts[iOrig]--;
				dGain = 0.0;
				for(iD=0;iD<iDims;iD++) dGain += KLDivApproxExclusiveProb(vDistribs[iOrig][iD],vDistribs[iClusts+1][iD]);
				if(bUseCounts)
					vcInf[iOrig] = ((prob_t)vCounts[iOrig] / (prob_t)iRows) * dGain;
				else
					vcInf[iOrig] = dGain;
			}
		}
		//didn't move any vertices, so done
		if(!bMoved) break;

		int i = 0;
		for (i = 0; i < vClustIDs.size(); i++)
			if (vSaved[i]!=vClustIDs[i]) break;
		if (i==vClustIDs.size())
			break; /* Identical solution found; break out of this loop */
	}

	iV = 0;
	MY_STACK::iterator Index;
	for(Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++)
	{
		CVertex* vertex = (CVertex*)*Index;
		if(vertex->GetNoise())continue;
		vertex->SetInfoClust(vClustIDs[iV]);
		iV++;
	}
}

vector<prob_t> CCluster::EntropyInfoClust(CVerxStack& DataStack,int iClusts,int iAlgorithmRuns,const CUPDUPDATA* pCUPDUPData)
{	char str[1024];
	sprintf(str,"EntropyInfoClust: iClusts=%d iAlgRuns=%d Sz=%d",iClusts,iAlgorithmRuns,DataStack.m_VerxStack.size());
	ScopedTimer S(str);

	extern int pBins[4];
	const int iBins = pBins[2]; //initialized to 30, but may be modified by user
	int k = 0;

	int iDims=DataStack.GetAutoClusteringDimension() , iD=0;

	vector< prob_t > vInfo(iClusts+1);

	m_pNumClusts[CLUST_INFO] = iClusts;

	vector<int> vClustIDsBest(DataStack.NumNonNoiseVertices());

	int iRows = 0, iCols = 0;
	int** pBinData = DataStack.GetVBinIDs<int>(true,iRows,iCols,iBins);
	AutoFree2D<int> AFData(pBinData);

	//best entropy of clusters found
	vector<prob_t> vcEntropyBest(iClusts+1,MAX_PROB_T / (iClusts+1));

	int iRun = 0;

	int iNNVertices = DataStack.NumNonNoiseVertices();

	//used to skip Peak-V, Valley-V, Valley
	vector<int> vZeroes(iDims);
	for(k=4;k<=7;k++)   vZeroes[k]=1; // skips Peak-V
	for(k=8;k<=11;k++)  vZeroes[k]=1; // skips Valley
	for(k=12;k<=15;k++) vZeroes[k]=1; // skips Valley-V
	k=0;

	for(iRun = 0; iRun < iAlgorithmRuns && !pCUPDUPData->ShouldTerminate(); iRun++)
	{
		char msg[1024];
		sprintf(msg,"Entropy reduction iteration %d of %d for %d clusters",iRun+1,iAlgorithmRuns,iClusts);
		pCUPDUPData->SetProgress(msg,99.0*(prob_t)iRun/iAlgorithmRuns);
		//entropy of clusters
		vector<prob_t> vcEntropy(iClusts+1);

		vector<int> vClustIDs(iNNVertices), vCounts(iClusts+1);

		randomassign(iClusts,vClustIDs.size(),&vClustIDs[0]);
		//make it 1-based indexing and init counts
		for(k=0;k<vClustIDs.size();k++)
		{
			vClustIDs[k]++;
			vCounts[vClustIDs[k]]++;
		}

		k=0;

		//fill distributions once
		vector< vector< Hist > > vDistribs;
		FillDistribs(DataStack,pBinData,*this,iBins,vDistribs,iClusts+1,vClustIDs);

		int iV = 0;

		//only need this for first time, since its recalculated for distributions who's
		//points change, when they change...
		CalcEntropy(vDistribs,vcEntropy,iClusts,iDims,vCounts,iRows,vZeroes,true,false);

		int iIter = 0;
		int iPeriod = 10;
		
		vector<int> vSaved(vClustIDs.size());

		k = 0;

		prob_t dSumE = Sum(vcEntropy), dLastSumE = dSumE;

		while(true)
		{
			if(pCUPDUPData->ShouldTerminate()) break;
			
			if (iIter % iPeriod == 0) /* Save the current cluster assignments */
			{
				int i;
				for (i = 0; i < vClustIDs.size(); i++) vSaved[i] = vClustIDs[i];
				if (iPeriod < INT_MAX / 2) iPeriod *= 2;
			}

			++iIter;

			bool bMoved = false;

			int iV = 0, iJnk = 0;

			for(iV=0;iV<vClustIDs.size();iV++,iJnk++)
			{
				if(iJnk % 200 == 0)
				{
					sprintf(msg,"Iteration %d of %d for %d clusters, sub-iteration %d",iRun+1,iAlgorithmRuns,iClusts,iIter);
					pCUPDUPData->SetProgress(msg,99.0*iJnk / vClustIDs.size());
				}
				int iOrig = vClustIDs[iV] , iT = 1;

				if(vCounts[iOrig] <= 1) continue; //don't make any empty clusters

				vector<prob_t> vEntropySums(iClusts+1);
				
				vEntropySums[iOrig] = Sum(vcEntropy);

				RemoveVertex(vDistribs[iOrig],pBinData[iV],iDims);

				for(iT=1;iT<=iClusts;iT++)
				{
					if(iT == iOrig) continue;

					//move vertex INTO temporary distribution
					AddVertex(vDistribs[iT],pBinData[iV],iDims);

					vector<prob_t> vEntropyTemp(vcEntropy);

					vEntropyTemp[iT] =    ((vCounts[iT] + 1.0)/(prob_t)iRows) * SumEntropy(vDistribs[iT],vZeroes);
					vEntropyTemp[iOrig] = ((vCounts[iOrig] - 1.0)/(prob_t)iRows) * SumEntropy(vDistribs[iOrig],vZeroes);
					
					vEntropySums[iT] = Sum(vEntropyTemp);

					//move vertex OUT of temporary distribution
					RemoveVertex(vDistribs[iT],pBinData[iV],iDims);
				}

				AddVertex(vDistribs[iOrig],pBinData[iV],iDims);

				//now see if moving the vertex to a different cluster had any reduction in entropy
				int iMinInd = iOrig; prob_t dMinEntropy = vEntropySums[iOrig];
				for(iT=1;iT<=iClusts;iT++)
				{
					if(iT==iOrig) continue;
					if(vEntropySums[iT] < dMinEntropy)
					{
						dMinEntropy = vEntropySums[iT];
						iMinInd = iT;
					}
				}

				if(iMinInd != iOrig)
				{
					//move vertex INTO destination distribution
					AddVertex(vDistribs[iMinInd],pBinData[iV],iDims);
					vCounts[iMinInd]++;
					vcEntropy[iMinInd] = ( (prob_t)vCounts[iMinInd] / (prob_t)iRows) * SumEntropy(vDistribs[iMinInd],vZeroes);

					//move vertex OUT of original distribution
					RemoveVertex(vDistribs[iOrig],pBinData[iV],iDims);
					vCounts[iOrig]--;
					vcEntropy[iOrig] = ( (prob_t)vCounts[iOrig] / (prob_t)iRows) * SumEntropy(vDistribs[iOrig],vZeroes);

					vClustIDs[iV] = iMinInd;

					bMoved = true;
				}
			}
			//didn't move any vertices, so done
			if(!bMoved) { Write2Log("NoMoves Final iIter=%d",iIter); break; }

			int i = 0;
			for (i = 0; i < vClustIDs.size(); i++)
				if (vSaved[i]!=vClustIDs[i]) break;
			if (i==vClustIDs.size())
			{ 	Write2Log("SameSolution Final iIter=%d",iIter); break; }// Identical solution found; break out of this loop
			
			dSumE = Sum(vcEntropy);
		}

		if(Sum(vcEntropy) < Sum(vcEntropyBest))
		{
			vcEntropyBest = vcEntropy;
			vClustIDsBest = vClustIDs;
		}
	}

	int iV = 0;
	MY_STACK::iterator Index;
	for(Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++)
	{
		CVertex* vertex = (CVertex*)*Index;
		if(vertex->GetNoise())continue;
		vertex->SetInfoClust(vClustIDsBest[iV]);
		iV++;
	}

	return vcEntropyBest;
}

struct binv
{
	char b[30];
	short count;
	binv()
	{
		int i;
		for(i=0;i<30;i++) b[i]=0;
		count=0;
	}
};

bool operator==(const binv& b1,const binv& b2)
{
	int i;
	for(i=0;i<30;i++) if(b1.b[i]!=b2.b[i]) return false;
	return true;
}

int Count(myhash& H)
{
	HIT IT;
	int cnt = 0;
	for(IT=H.begin();IT!=H.end();IT++)
		cnt += IT->second;
	return cnt;
}

prob_t Entropy(myhash& H)
{
	HIT IT;
	prob_t dEntrop = 0;
	double sz = Count(H);
	for(IT=H.begin();IT!=H.end();IT++)
	{
		if(IT->second<=0) continue;
		double dv = IT->second / sz;
		dEntrop += dv * log2(dv);
	}
	return -dEntrop;
}

prob_t Entropy(myhash& H,double sz,double maxbin)
{
	HIT IT;
	prob_t dEntrop = 0;
	IT=H.begin();
//	int dims = strlen(IT->first);
	//A2D<int> dimcnts(dims,maxbin+1);
	//dimcnts.Fill(0);
//	int s = 0;
//	vector<int> counts(H.size());
//	vector<double> weights(H.size());
	for(;IT!=H.end();IT++)
	{
		if(IT->second<=0) continue;

//		counts.resize(0); weights.resize(0);

//		counts.push_back(IT->second);
//		weights.push_back(0.5);

		double dv = IT->second / sz;
		
		dEntrop += dv * log2(dv);
		
		//for(s=0;s<dims;s++)
		//	dimcnts[s][IT->first[s]-'0']++;

//		HIT JT = H.begin();
//		for(;JT!=H.end();JT++)
//		{
//			if(JT==IT) continue;
//			int sd = 0;
//			for(s=0;s<dims;s++)
//				if(abs(IT->first[s]-JT->first[s])>1)
//					break;
//			if(s==dims)
//				counts.push_back(JT->second);
//		}
//		int sc = Sum(counts);
//		for(s=0;s<counts.size();s++)
//			weights[s] = counts[s] / (double)sc;
//		for(s=0;s<counts.size();s++)
//			dv += counts[s] * weights[s] 
	}
	/*for(s=0;s<dims;s++)
	{
		int b , mb = maxbin;
		for(b=0;b<mb;b++)
		{
			if(dimcnts[s][b])
			{	
				double d = dimcnts[s][b] / sz;
				dEntrop += d * log2(d);
			}
		}
	}*/
	return -dEntrop;
}

void RemoveVertex(myhash& H, const char* str)
{
	HIT IT = H.find(str);
	if(IT==H.end())
		return;
	if((*IT).second > 1) 
		(*IT).second -= 1; 
	else H.erase(str);
}

void AddVertex(myhash& H, const char* str)
{
	HIT IT = H.find(str);
	if(IT==H.end()) 
		H.insert(pair<const char*,int>(str,1));
		//H[str]=1;
	else 
		(*IT).second += 1;
}

void PrintH(myhash& H)
{
	Write2LogPlain("Hash:");
	HIT IT;
	for(IT=H.begin();IT!=H.end();IT++)
		Write2LogPlain("%s : %d",IT->first,IT->second);
	Write2LogPlain("\n");
}


vector<prob_t> CCluster::EntropyInfoClustMD(CVerxStack& DataStack,int iClusts,int iAlgorithmRuns,const CUPDUPDATA* pCUPDUPData)
{	char str[1024];
	sprintf(str,"EntropyInfoClust: iClusts=%d iAlgRuns=%d Sz=%d",iClusts,iAlgorithmRuns,DataStack.m_VerxStack.size());
	ScopedTimer S(str);

	extern int pBins[4];
	const int iBins = pBins[2]; //initialized to 30, but may be modified by user
	int k = 0;

	int iDims=DataStack.GetAutoClusteringDimension() , iD=0;

	vector< prob_t > vInfo(iClusts+1);

	m_pNumClusts[CLUST_INFO] = iClusts;

	vector<int> vClustIDsBest(DataStack.NumNonNoiseVertices());

	int iRows = 0, iCols = 0;

	vector<float> vFloat,vRange;
	DataStack.GetFloatV(iRows,iCols,vFloat,vRange,false);

	vector<int> dims2use(8);
	int qwert = 0;
	for(qwert=0;qwert<dims2use.size();qwert++)
		dims2use[qwert]=qwert;
	if(dims2use.size()>=8)
	{	dims2use[4]=16;
		dims2use[5]=17;
		dims2use[6]=18;
		dims2use[7]=19;
	}

	A2D<char> vStrs(iRows,dims2use.size()+1);

	double maxbin = 3.0;

	for(k=0;k<iRows;k++)
	{
		int t = 0;
		for(t=0;t<dims2use.size();t++)
			vStrs[k][t] = '0' + char(vFloat[k*iCols+dims2use[t]] * maxbin + 0.5);
		vStrs[k][t]=0;
	}

	vector< myhash > IDs(iClusts+1);

	//best entropy of clusters found
	vector<prob_t> vcEntropyBest(iClusts+1,MAX_PROB_T / (iClusts+1));

	int iRun = 0;

	int iNNVertices = DataStack.NumNonNoiseVertices();

	//used to skip Peak-V, Valley-V, Valley
	vector<int> vZeroes(iDims);
	for(k=4;k<=7;k++)   vZeroes[k]=1; // skips Peak-V
	for(k=8;k<=11;k++)  vZeroes[k]=1; // skips Valley
	for(k=12;k<=15;k++) vZeroes[k]=1; // skips Valley-V
	k=0;

	for(iRun = 0; iRun < iAlgorithmRuns && !pCUPDUPData->ShouldTerminate(); iRun++)
	{
		char msg[1024];
		sprintf(msg,"Entropy reduction iteration %d of %d for %d clusters",iRun+1,iAlgorithmRuns,iClusts);
		pCUPDUPData->SetProgress(msg,99.0*(prob_t)iRun/iAlgorithmRuns);
		//entropy of clusters
		vector<prob_t> vcEntropy(iClusts+1);

		vector<int> vClustIDs(iNNVertices), vCounts(iClusts+1);

		randomassign(iClusts,vClustIDs.size(),&vClustIDs[0]);
		//make it 1-based indexing and init counts
		for(k=0;k<vClustIDs.size();k++)
		{
			vClustIDs[k]++;
			vCounts[vClustIDs[k]]++;
		}

		k=0;

		//fill distributions once
		int c,r;
		for(c=1;c<=iClusts;c++)
		{	myhash& H = IDs[c];
			for(r=0;r<iRows;r++)
				if(vClustIDs[r]==c)
					AddVertex(H,vStrs[r]);
			Write2LogPlain("original random hash %d : : :\n",c);
			PrintH(H);
			Write2LogPlain("");
		}

		int iV = 0;

		//only need this for first time, since its recalculated for distributions who's
		//points change, when they change...
		for(c=1;c<=iClusts;c++)
			vcEntropy[c] = ( vCounts[c] / (prob_t) iRows ) * Entropy(IDs[c],vCounts[c],maxbin);

		int iIter = 0;
		int iPeriod = 10;
		
		vector<int> vSaved(vClustIDs.size());

		k = 0;

		prob_t dSumE = Sum(vcEntropy), dLastSumE = dSumE;

		while(true)
		{
			if(pCUPDUPData->ShouldTerminate()) break;
			
			if (iIter % iPeriod == 0) /* Save the current cluster assignments */
			{
				int i;
				for (i = 0; i < vClustIDs.size(); i++) vSaved[i] = vClustIDs[i];
				if (iPeriod < INT_MAX / 2) iPeriod *= 2;
			}

			++iIter;

			bool bMoved = false;

			int iV = 0, iJnk = 0;

			for(iV=0;iV<vClustIDs.size();iV++,iJnk++)
			{
				if(iJnk % 200 == 0)
				{
					sprintf(msg,"Iteration %d of %d for %d clusters, sub-iteration %d",iRun+1,iAlgorithmRuns,iClusts,iIter);
					pCUPDUPData->SetProgress(msg,99.0*iJnk / vClustIDs.size());
				}
				int iOrig = vClustIDs[iV] , iT = 1;

				if(vCounts[iOrig] <= 1) continue; //don't make any empty clusters

				vector<prob_t> vEntropySums(iClusts+1);
				
				vEntropySums[iOrig] = Sum(vcEntropy);

				RemoveVertex(IDs[iOrig],vStrs[iV]);

				for(iT=1;iT<=iClusts;iT++)
				{
					if(iT == iOrig) continue;

					//move vertex INTO temporary distribution
					AddVertex(IDs[iT],vStrs[iV]);

					vector<prob_t> vEntropyTemp(vcEntropy);

					vEntropyTemp[iT] =    ((vCounts[iT] + 1.0)/(prob_t)iRows) * Entropy(IDs[iT],vCounts[iT]+1,maxbin); 
					vEntropyTemp[iOrig] = ((vCounts[iOrig] - 1.0)/(prob_t)iRows) * Entropy(IDs[iOrig],vCounts[iOrig]-1,maxbin); 
					
					vEntropySums[iT] = Sum(vEntropyTemp);

					//move vertex OUT of temporary distribution
					RemoveVertex(IDs[iT],vStrs[iV]);
				}

				AddVertex(IDs[iOrig],vStrs[iV]);

				//now see if moving the vertex to a different cluster had any reduction in entropy
				int iMinInd = iOrig; prob_t dMinEntropy = vEntropySums[iOrig];
				for(iT=1;iT<=iClusts;iT++)
				{
					if(iT==iOrig) continue;
					if(vEntropySums[iT] < dMinEntropy)
					{
						dMinEntropy = vEntropySums[iT];
						iMinInd = iT;
					}
				}

				if(iMinInd != iOrig)
				{
					//move vertex INTO destination distribution
					AddVertex(IDs[iMinInd],vStrs[iV]);
					vCounts[iMinInd]++;
					vcEntropy[iMinInd] = ( (prob_t)vCounts[iMinInd] / (prob_t)iRows) * Entropy(IDs[iMinInd],vCounts[iMinInd],maxbin); 

					//move vertex OUT of original distribution
					RemoveVertex(IDs[iOrig],vStrs[iV]);
					vCounts[iOrig]--;
					vcEntropy[iOrig] = ( (prob_t)vCounts[iOrig] / (prob_t)iRows) * Entropy(IDs[iOrig],vCounts[iOrig],maxbin);

					vClustIDs[iV] = iMinInd;

					bMoved = true;
				}
			}
			//didn't move any vertices, so done
			if(!bMoved) { Write2Log("NoMoves Final iIter=%d",iIter); break; }

			if(Sum(vcEntropy) < Sum(vcEntropyBest))
			{
				vcEntropyBest = vcEntropy;
				vClustIDsBest = vClustIDs;
			}

			int i = 0;
			for (i = 0; i < vClustIDs.size(); i++)
				if (vSaved[i]!=vClustIDs[i]) break;
			if (i==vClustIDs.size())
			{ 	Write2Log("SameSolution Final iIter=%d",iIter); break; }// Identical solution found; break out of this loop
			
			dSumE = Sum(vcEntropy);
		}
	}

	int iV = 0;
	MY_STACK::iterator Index;
	for(Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++)
	{
		CVertex* vertex = (CVertex*)*Index;
		if(vertex->GetNoise())continue;
		vertex->SetInfoClust(vClustIDsBest[iV]);
		iV++;
	}

	int c;
	for(c=1;c<=iClusts;c++)
	{	myhash& H = IDs[c];
		Write2LogPlain("final hash %d : : :\n",c);
		PrintH(H);
		Write2LogPlain("...");
	}


	return vcEntropyBest;
}

#ifdef DONTCOMPILE
vector<prob_t> CCluster::EntropyInfoClustMD(CVerxStack& DataStack,int iClusts,int iAlgorithmRuns,const CUPDUPDATA* pCUPDUPData)
{	char str[1024];
	sprintf(str,"EntropyInfoClust: iClusts=%d iAlgRuns=%d Sz=%d",iClusts,iAlgorithmRuns,DataStack.m_VerxStack.size());
	ScopedTimer S(str);

	extern int pBins[4];
	const int iBins = pBins[2]; //initialized to 30, but may be modified by user
	int k = 0;

	int iDims=DataStack.GetAutoClusteringDimension() , iD=0;

	vector< prob_t > vInfo(iClusts+1);

	m_pNumClusts[CLUST_INFO] = iClusts;

	vector<int> vClustIDsBest(DataStack.NumNonNoiseVertices());

	int iRows = 0, iCols = 0;

	vector<float> vFloat,vRange;
	DataStack.GetFloatV(iRows,iCols,vFloat,vRange,false);

	vector<int> dims2use(8);
	int qwert = 0;
	for(qwert=0;qwert<dims2use.size();qwert++)
		dims2use[qwert]=qwert;
	dims2use[4]=16;
	dims2use[5]=17;
	dims2use[6]=18;
	dims2use[7]=19;

	A2D<char> vStrs(iRows,dims2use.size()+1);

	double maxbin = 3.0;

	for(k=0;k<iRows;k++)
	{
		int t = 0;
		for(t=0;t<dims2use.size();t++)
			vStrs[k][t] = '0' + char(vFloat[k*iCols+dims2use[t]] * maxbin + 0.5);
		vStrs[k][t]=0;
	}

	vector< myhash > IDs(iClusts+1);

	//best entropy of clusters found
	vector<prob_t> vcEntropyBest(iClusts+1,MAX_PROB_T / (iClusts+1));

	int iRun = 0;

	int iNNVertices = DataStack.NumNonNoiseVertices();

	//used to skip Peak-V, Valley-V, Valley
	vector<int> vZeroes(iDims);
	for(k=4;k<=7;k++)   vZeroes[k]=1; // skips Peak-V
	for(k=8;k<=11;k++)  vZeroes[k]=1; // skips Valley
	for(k=12;k<=15;k++) vZeroes[k]=1; // skips Valley-V
	k=0;

	for(iRun = 0; iRun < iAlgorithmRuns && !pCUPDUPData->ShouldTerminate(); iRun++)
	{
		char msg[1024];
		sprintf(msg,"Entropy reduction iteration %d of %d for %d clusters",iRun+1,iAlgorithmRuns,iClusts);
		pCUPDUPData->SetProgress(msg,99.0*(prob_t)iRun/iAlgorithmRuns);
		//entropy of clusters
		vector<prob_t> vcEntropy(iClusts+1);

		vector<int> vClustIDs(iNNVertices,0), vCounts(iClusts+1);

		//randomassign(iClusts,vClustIDs.size(),&vClustIDs[0]);
		//make it 1-based indexing and init counts
		//for(k=0;k<vClustIDs.size();k++)
		{
		//	vClustIDs[k]++;
		//	vCounts[vClustIDs[k]]++;
		}

		k=0;

		//fill distributions once
		int c,r;
		for(c=1;c<=iClusts;c++)
		{	myhash& H = IDs[c];
			//for(r=0;r<iRows;r++)
				//if(vClustIDs[r]==c)
			r = c - 1;
			AddVertex(H,vStrs[r]);
			Write2LogPlain("original random hash %d : : :\n",c);
			PrintH(H);
			Write2LogPlain("");
			vCounts[c]=1;
		}

		int iV = 0;

		//only need this for first time, since its recalculated for distributions who's
		//points change, when they change...
		for(c=1;c<=iClusts;c++)
			vcEntropy[c] = 0; //( vCounts[c] / (prob_t) iRows ) * Entropy(IDs[c],vCounts[c],maxbin);

		int iIter = 0;
		int iPeriod = 10;
		
		vector<int> vSaved(vClustIDs.size());

		k = 0;

		prob_t dSumE = Sum(vcEntropy), dLastSumE = dSumE;

		while(true)
		{
			if(pCUPDUPData->ShouldTerminate()) break;
			
			if (iIter % iPeriod == 0) /* Save the current cluster assignments */
			{
				int i;
				for (i = 0; i < vClustIDs.size(); i++) vSaved[i] = vClustIDs[i];
				if (iPeriod < INT_MAX / 2) iPeriod *= 2;
			}

			++iIter;

			bool bMoved = false;

			int iV = 0, iJnk = 0;

			//for(iV=0;iV<vClustIDs.size();iV++,iJnk++)
			for(iV=0;iV<vClustIDs.size();iV++,iJnk++)
			{
				if(iJnk % 200 == 0)
				{
					sprintf(msg,"Iteration %d of %d for %d clusters, sub-iteration %d",iRun+1,iAlgorithmRuns,iClusts,iIter);
					pCUPDUPData->SetProgress(msg,99.0*iJnk / vClustIDs.size());
				}
				int iOrig = vClustIDs[iV] , iT = 1;

				//if(iOrig!=0) continue;

				vector<prob_t> vEntropyDiff(iClusts+1,0.0f);

				if( iOrig == 0 )
				{
					for(iT=1;iT<=iClusts;iT++)
					{
						//move vertex INTO temporary distribution
						AddVertex(IDs[iT],vStrs[iV]);

						vector<prob_t> vEntropyTemp(vcEntropy);

						vEntropyDiff[iT] =   vcEntropy[iT] - ((vCounts[iT] + 1.0)/(prob_t)iRows) * Entropy(IDs[iT],vCounts[iT]+1,maxbin); 						

						//move vertex OUT of temporary distribution
						RemoveVertex(IDs[iT],vStrs[iV]);
					}
					//now see if moving the vertex to a different cluster had any reduction in entropy
					prob_t dMinEntropyDiff = 1e6; int iMinInd = 0;
					for(iT=1;iT<=iClusts;iT++)
					{
						if(vEntropyDiff[iT] < dMinEntropyDiff)
						{
							dMinEntropyDiff = vEntropyDiff[iT];
							iMinInd = iT;
						}
					}

					//if(iMinInd != iOrig)
					{
						//move vertex INTO destination distribution
						AddVertex(IDs[iMinInd],vStrs[iV]);
						vCounts[iMinInd]++;
						vcEntropy[iMinInd] = ( (prob_t)vCounts[iMinInd] / (prob_t)iRows) * Entropy(IDs[iMinInd],vCounts[iMinInd],maxbin); 

						//move vertex OUT of original distribution
						RemoveVertex(IDs[iOrig],vStrs[iV]);
						vCounts[iOrig]--;
						vcEntropy[iOrig] = ( (prob_t)vCounts[iOrig] / (prob_t)iRows) * Entropy(IDs[iOrig],vCounts[iOrig],maxbin);

						vClustIDs[iV] = iMinInd;

						bMoved = true;
					}
				}
				else
				{
					if(vCounts[iOrig] <= 1) continue; //don't make any empty clusters

					vector<prob_t> vEntropySums(iClusts+1);
					
					vEntropySums[iOrig] = Sum(vcEntropy);

					RemoveVertex(IDs[iOrig],vStrs[iV]);

					for(iT=1;iT<=iClusts;iT++)
					{
						if(iT == iOrig) continue;

						//move vertex INTO temporary distribution
						AddVertex(IDs[iT],vStrs[iV]);

						vector<prob_t> vEntropyTemp(vcEntropy);

						vEntropyTemp[iT] =    ((vCounts[iT] + 1.0)/(prob_t)iRows) * Entropy(IDs[iT],vCounts[iT]+1,maxbin); 
						vEntropyTemp[iOrig] = ((vCounts[iOrig] - 1.0)/(prob_t)iRows) * Entropy(IDs[iOrig],vCounts[iOrig]-1,maxbin); 
						
						vEntropySums[iT] = Sum(vEntropyTemp);

						//move vertex OUT of temporary distribution
						RemoveVertex(IDs[iT],vStrs[iV]);
					}

					AddVertex(IDs[iOrig],vStrs[iV]);

					//now see if moving the vertex to a different cluster had any reduction in entropy
					int iMinInd = iOrig; prob_t dMinEntropy = vEntropySums[iOrig];
					for(iT=1;iT<=iClusts;iT++)
					{
						if(iT==iOrig) continue;
						if(vEntropySums[iT] < dMinEntropy)
						{
							dMinEntropy = vEntropySums[iT];
							iMinInd = iT;
						}
					}

					if(iMinInd != iOrig)
					{
						//move vertex INTO destination distribution
						AddVertex(IDs[iMinInd],vStrs[iV]);
						vCounts[iMinInd]++;
						vcEntropy[iMinInd] = ( (prob_t)vCounts[iMinInd] / (prob_t)iRows) * Entropy(IDs[iMinInd],vCounts[iMinInd],maxbin); 

						//move vertex OUT of original distribution
						RemoveVertex(IDs[iOrig],vStrs[iV]);
						vCounts[iOrig]--;
						vcEntropy[iOrig] = ( (prob_t)vCounts[iOrig] / (prob_t)iRows) * Entropy(IDs[iOrig],vCounts[iOrig],maxbin);

						vClustIDs[iV] = iMinInd;

						bMoved = true;
					}
				}
			}

			if(iIter == 1){
				vcEntropyBest = vcEntropy;
				vClustIDsBest = vClustIDs;
			}
			//break;

			//didn't move any vertices, so done
			if(!bMoved) { Write2Log("NoMoves Final iIter=%d",iIter); break; }

			if(Sum(vcEntropy) < Sum(vcEntropyBest))
			{
				vcEntropyBest = vcEntropy;
				vClustIDsBest = vClustIDs;
			}

			int i = 0;
			for (i = 0; i < vClustIDs.size(); i++)
				if (vSaved[i]!=vClustIDs[i]) break;
			if (i==vClustIDs.size())
			{ 	Write2Log("SameSolution Final iIter=%d",iIter); break; }// Identical solution found; break out of this loop
			
			dSumE = Sum(vcEntropy);
		}
	}

	int iV = 0;
	MY_STACK::iterator Index;
	for(Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++)
	{
		CVertex* vertex = (CVertex*)*Index;
		if(vertex->GetNoise())continue;
		vertex->SetInfoClust(vClustIDsBest[iV]);
		iV++;
	}

	int c;
	for(c=1;c<=iClusts;c++)
	{	myhash& H = IDs[c];
		Write2LogPlain("final hash %d : : :\n",c);
		PrintH(H);
		Write2LogPlain("...");
	}


	return vcEntropyBest;
}
#endif

vector<prob_t> CCluster::EntropyInfoClustKDTree(CVerxStack& DataStack,int iClusts,int iAlgorithmRuns,const CUPDUPDATA* pCUPDUPData)
{	
	int iDims=DataStack.GetAutoClusteringDimension(), iD=0;

	vector< prob_t > vInfo(iClusts+1);

	m_pNumClusts[CLUST_INFO] = iClusts;

	vector<int> vClustIDsBest(DataStack.NumNonNoiseVertices());

	//get all vertex data as a single continuous float vector
	int iRows = 0, iCols = 0;
	vector<float> vFloat , vRange;
	DataStack.GetFloatV(iRows,iCols,vFloat,vRange,true);
	iDims = iCols;

	int k = 0;

	char msg[1024];

	////////////////////////////////////////////////////////
	//find nearest neighbor for each vertex
	const int iNNToFind = m_iNNToFind;;
	KDTreeHist oTree;
	oTree.SetData(iCols,&vFloat[0],iRows);
	vector< Neighbor > vnn(iRows);
	sprintf(msg,"Finding initial %d nearest neighbors...",iNNToFind);
	pCUPDUPData->SetProgress(msg);
	{//ScopedTimer S("find 10 nearest neighbors total time");
	for(k=0;k<vnn.size();k++)
	{
		oTree.GetNearestNeighbor(oTree[k],false,vnn[k]);
	}
	}
	A2D<Neighbor> vNeighbors(iRows,1);
	////////////////////////////////////////////////////////

	//best entropy of clusters found
	vector<prob_t> vcEntropyBest(iClusts+1,MAX_PROB_T / (iClusts+1));

	int iRun = 0;

	int iNNVertices = DataStack.NumNonNoiseVertices();

	vector<int> vCounts(iClusts+1);

	vector<int> vnCount(iRows,1);

	for(iRun = 0; iRun < iAlgorithmRuns && !pCUPDUPData->ShouldTerminate(); iRun++)
	{
		sprintf(msg,"Entropy reduction iteration %d of %d for %d clusters",iRun+1,iAlgorithmRuns,iClusts);
		pCUPDUPData->SetProgress(msg,99.0*(prob_t)iRun/iAlgorithmRuns);

		//entropy of clusters
		vector<prob_t> vcEntropy(iClusts+1);

		vector<int> vClustIDs(DataStack.m_VerxStack.size());
		vCounts=vector<int>(iClusts+1);

		bool bInitKM = false;
		if(bInitKM)
		{
			KmeansClust(DataStack,iClusts,iClusts,1,1,false,false,pCUPDUPData);
			int iV = 0;
			MY_STACK::iterator Index;
			for(Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++,iV++)
			{
				CVertex* vertex = (CVertex*)*Index;
				if(vertex->GetNoise())continue;
				vClustIDs[iV] = vertex->GetKmeansClust();
			}
			//make it 1-based indexing and init counts
			for(k=0;k<vClustIDs.size();k++)
				vCounts[vClustIDs[k]]++;
		}
		else
		{
			randomassign(iClusts,vClustIDs.size(),&vClustIDs[0]);
			//make it 1-based indexing and init counts
			for(k=0;k<vClustIDs.size();k++)
			{
				vClustIDs[k]++;
				vCounts[vClustIDs[k]]++;
			}
		}

		vector< KDTreeHist > vTree(iClusts+1), vCompTree(iClusts+1);

		A2D<int> vBestDims(iClusts+1,2);
		{ int i,j;
		for(i=1;i<=iClusts;i++)
			for(j=0;j<vBestDims.Cols();j++)
				vBestDims[i][j]=j;
		//FillDistribs(
		FillDistribs(vFloat,vTree,vCompTree,iClusts+1,vClustIDs,vCounts,iCols,vBestDims,vBestDims.Cols());
		}

		//one kdtree for each cluster
		pCUPDUPData->SetProgress("Building KDTrees...");
		{ //ScopedTimer S("build all trees");
			//for(k=1;k<=iClusts;k++)
			{
			//	vTree[k].SetData(vFloat,vClustIDs,vCounts[k],iDims,k);
			}
		}

		k=0;

		int iV = 0;

		//only need this for first time, since its recalculated for distributions who's
		//points change, when they change...
		for(k=1;k<=iClusts;k++)
			vcEntropy[k]=(vCounts[k]/(prob_t)iRows)*Entropy(vTree[k],vnn,k,vClustIDs);
		
		int iIter = 0;
		int iPeriod = 10;
		
		vector<int> vSaved(vClustIDs.size());

		k = 0;

		prob_t dSumE = Sum(vcEntropy), dLastSumE = dSumE;

		while(true)
		{
			if(pCUPDUPData->ShouldTerminate()) break;
			
			if (iIter % iPeriod == 0) /* Save the current cluster assignments */
			{
				int i;
				for (i = 0; i < vClustIDs.size(); i++) vSaved[i] = vClustIDs[i];
				if (iPeriod < INT_MAX / 2) iPeriod *= 2;
			}

			++iIter;

			bool bMoved = false;

			int iV = 0, iJnk = 0;

			for(iV=0;iV<vClustIDs.size();iV++,iJnk++)
			{
				//if(1 || iJnk % 200 == 0)
				{
					sprintf(msg,"Iteration %d of %d for %d clusters, sub-iteration %d v%d",iRun+1,iAlgorithmRuns,iClusts,iIter,iJnk);
					pCUPDUPData->SetProgress(msg,99.0*iJnk / vClustIDs.size());
				}
				
				int iOrig = vClustIDs[iV] , iT = 1;

				if(vCounts[iOrig] <= 2) continue; //need >= 2 elements in each cluster!!

				vector<prob_t> vEntropySums(iClusts+1);
				
				vEntropySums[iOrig] = Sum(vcEntropy);

				vector<prob_t> vEntropyTempA(vcEntropy);
				vClustIDs[iV] = 100000; //move out of orig
				vTree[iOrig].SetData(vFloat,vClustIDs,vCounts[iOrig]-1,iDims,iOrig);
				vEntropyTempA[iOrig]=((vCounts[iOrig]-1.0)/(prob_t)iRows)*Entropy(vTree[iOrig],vnn,iOrig,vClustIDs);				

				vector< KDTreeHist > vTreeTmp(iClusts+1);
				
				for(iT=1;iT<=iClusts;iT++)
				{
					if(iT == iOrig) continue;

					//move vertex INTO temporary distribution
					vClustIDs[iV]=iT;

					KDTreeHist oTreeTmp;
					oTreeTmp.SetData(vFloat,vClustIDs,vCounts[iT]+1,iDims,iT);

					vector<prob_t> vEntropyTempB(vEntropyTempA);

					vEntropyTempB[iT]=((vCounts[iT]+1.0)/(prob_t)iRows)*Entropy(oTreeTmp,vnn,iT,vClustIDs);
					
					vEntropySums[iT] = Sum(vEntropyTempB);
				}

				//put vertex back into original distrib
				vClustIDs[iV]=iOrig;

				//now see if moving the vertex to a different cluster had any reduction in entropy
				int iMinInd = iOrig; prob_t dMinEntropy = vEntropySums[iOrig];//0.0;
				for(iT=1;iT<=iClusts;iT++)
				{
					if(iT==iOrig) continue;
					if(vEntropySums[iT] < dMinEntropy)
					{
						dMinEntropy = vEntropySums[iT];
						iMinInd = iT;
					}
				}

				if(iMinInd != iOrig)
				{
					//move vertex INTO destination distribution
					vClustIDs[iV]=iMinInd;
					vCounts[iMinInd]++;
					vTree[iMinInd].SetData(vFloat,vClustIDs,vCounts[iMinInd],iDims,iMinInd);
					vcEntropy[iMinInd]=((prob_t)vCounts[iMinInd]/(prob_t)iRows)*Entropy(vTree[iMinInd],vnn,iMinInd,vClustIDs);

					//move vertex OUT of original distribution
					vCounts[iOrig]--;
					vTree[iOrig].SetData(vFloat,vClustIDs,vCounts[iOrig],iDims,iOrig);
					vcEntropy[iOrig]=((prob_t)vCounts[iOrig]/(prob_t)iRows)*Entropy(vTree[iOrig],vnn,iOrig,vClustIDs);

					bMoved = true;

					prob_t dSumTmp = Sum(vcEntropy);

					dSumTmp = dSumTmp;
				}
				else
				{
					vTree[iOrig].SetData(vFloat,vClustIDs,vCounts[iOrig],iDims,iOrig);
				}
			}

			//didn't move any vertices, so done
			if(!bMoved) break;

			int i = 0;
			for (i = 0; i < vClustIDs.size(); i++)
				if (vSaved[i]!=vClustIDs[i]) break;
			if (i==vClustIDs.size())
				break; /* Identical solution found; break out of this loop */
			
			dSumE = Sum(vcEntropy);

			dSumE = dSumE;
		}

		if(Sum(vcEntropy) < Sum(vcEntropyBest))
		{
			vcEntropyBest = vcEntropy;
			vClustIDsBest = vClustIDs;
		}
	}

	int iV = 0;
	MY_STACK::iterator Index;
	for(Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++,iV++)
	{
		CVertex* vertex = (CVertex*)*Index;
		if(vertex->GetNoise())continue;
		vertex->SetInfoClust(vClustIDsBest[iV]);
	}

	CString cstr;
	for(int iC=1;iC<=iClusts;iC++)
	{
		cstr.Format("clust%d count=%d kld=%.12f",iC,vCounts[iC],vcEntropyBest[iC]);
		MessageBox(0,cstr.GetBuffer(),"WClust",0);
	}

	return vcEntropyBest;
}

bool CCluster::KlustaKwikClust(CVerxStack& DataStack,const CUPDUPDATA* pUp)
{
	if(pUp) pUp->SetProgress("Performing KlustaKwik clustering",0);
	//setup data file for klustakwik
	//i know its stupid to do it through disk
	//should change it to work through memory
	char* strKKInFname = "wclust_tmp_klusta_kwik_tmp.fet.1", *strKKOutFname = "wclust_tmp_klusta_kwik_tmp.clu.1";
	FILE* fp = fopen(strKKInFname,"w");
	if(!fp) return false;
	MY_STACK::iterator Index;
	char msg[1024];
	int iV = 0 , idx = 0 , iD = 0;
	pUp->SetProgress("Computing PCA for KlustaKwik clustering.");
	DataStack.DoVertexPCA(true);
	vector<float>& IData = DataStack.ICED();
	fprintf(fp,"8\n");
	for(iV=0;iV<DataStack.m_VerxStack.size() && (!pUp || !pUp->ShouldTerminate());iV++)
	{	
		pUp->SetProgress("Creating KlustaKwik data file", 99.0 * iV / DataStack.m_VerxStack.size());
		for(iD=0;iD<8;iD++) fprintf(fp,"%g ",IData[idx++]);
		fprintf(fp,"\n");
	}
	DataStack.ClearICED();
	fclose(fp);

	if(pUp && pUp->ShouldTerminate()) return false;

	const int argc = 5;
	char* argv[argc] = {"KlustaKwik","wclust_tmp_klusta_kwik_tmp","1","-MaxIter","1000"};
	kk_main(argc,argv,pUp);

	pUp->SetProgress("Saving KlustaKwik results",0);

	fp = fopen(strKKOutFname,"r");
	if(!fp) return false;
	int i;
	char tmp[1024];
	fgets(tmp,1024,fp);
	tmp[strlen(tmp)-1]=0;
	int iClusts = atoi(tmp);
	m_pNumClusts[CLUST_KK]=iClusts;
	Index=DataStack.m_VerxStack.begin();
	iV = 0;
	while(!feof(fp))
	{
		CVertex* verx = (CVertex*) *Index;
		while(Index!=DataStack.m_VerxStack.end() && verx->GetNoise())
		{
			Index++;
			verx = (CVertex*) *Index;
		}
		if(Index==DataStack.m_VerxStack.end()) break;
		fgets(tmp,1024,fp);
		tmp[strlen(tmp)-1]=0;
		if(feof(fp)) break;
		int iC = atoi(tmp);
		verx->SetKKClust(iC);
		iC = verx->GetKKClust();
		iV++;
		Index++;
		pUp->SetProgress( 99.0 * iV / DataStack.m_VerxStack.size() );
	}
	fclose(fp);
	
	return true;
}

void CCluster::APClust(CVerxStack& DataStack,int iIter,int iConvIter,prob_t dDamp,prob_t dPref,const CUPDUPDATA* pUp)
{
	pUp->SetProgress("Performing Affinity Propagation",0);
	
	vector<APNUM> vSimsPrefs;
	vector<unsigned long> vI,vK;

	int iRows=0,iCols=0;

	{
	//get vertices with values normalized btwn 0 and 1
	vector< vector<APNUM> > vN;	
	//DataStack.NormalizedV(vN,true);
	DataStack.GetV(vN,true);
	
	iRows = vN.size(); iCols = vN[0].size();
	
	//similarity matrix
	//vector< vector<APNUM> > vSim(iRows, vector<APNUM>(iRows));

	vSimsPrefs = vector<APNUM>(iRows*iRows);
	vI = vector<unsigned long>(iRows*iRows);
	vK = vector<unsigned long>(iRows*iRows);

	vSimsPrefs.resize(0); vI.resize(0); vK.resize(0);
	
	unsigned long i,j;
	
	//compute similarity matrix
	for(i=0;i<iRows && !pUp->ShouldTerminate();i++)
	{	pUp->SetProgress("Computing similarity matrix", 99.0 * i / iRows );
		for(j=0;j<iRows;j++)
		{
			if(i==j)
			{
				continue;
				//vSim[i][j] = 0.0;
			}
			else
			{
				APNUM dValTotal = 0.0;
				int k;
				for(k=0;k<iCols;k++)
				{
					APNUM dVal = vN[i][k] - vN[j][k];
					//vSim[i][j] -= dVal * dVal;
					dValTotal -= dVal * dVal;
				}

				vI.push_back(i);
				vK.push_back(j);
				//vSimsPrefs.push_back(vSim[i][j]);
				vSimsPrefs.push_back(dValTotal);
			}
		}
	}

	for(i=0;i<iRows;i++) vN[i].clear();
	vN.clear();
	vN=vector< vector<APNUM> >();

	if(pUp->ShouldTerminate()) return;

	//compute preferences, set it to median of similarity values * dPref
	vector<APNUM> vTmp(vSimsPrefs);//iRows*iRows);
	int k = 0;
	//for(i=0;i<iRows && !pUp->ShouldTerminate();i++)
	{	pUp->SetProgress("Computing preferences", 99.0 * i / iRows );
	//	for(j=0;j<iRows;j++)
		{
	//		vTmp[k++] = vSim[i][j];
		}
	}
	std::sort(vTmp.begin(),vTmp.end());
	APNUM dMed = vTmp[vTmp.size()/2];
	vTmp.clear();
	//vector<APNUM> vPref(iRows);
	//for(i=0;i<iRows;i++) vPref[i] = dMed;

	if(pUp->ShouldTerminate()) return;
/*
	FILE* fp = fopen("__tmp__wclust_sims.txt","w");
	//i k s (index i, index k, similarity between i and k)
	for(i=0;i<iRows && !pUp->ShouldTerminate();i++)
	{	pUp->SetProgress("Step 1 converting to AP data format", 99.0 * i / iRows );
		for(j=0;j<iRows;j++)
		{
			if(i==j) continue;
			fprintf(fp,"%d %d %f\n",i+1,j+1,vSim[i][j]);
		}
	}
	fclose(fp);
*/
	if(pUp->ShouldTerminate()) return;

//	fp = fopen("__tmp__wclust_prefs.txt","w");
	APNUM dPrefFinal = dMed * dPref;
	for(i=0;i<iRows && !pUp->ShouldTerminate();i++)
	{	//pUp->SetProgress("Step 2 converting to AP data format", 99.0 * i / iRows );
		vSimsPrefs.push_back(dPrefFinal);
		vI.push_back(i);
		vK.push_back(i);
//		fprintf(fp,"%f\n",dMed*dPref);
	}
//	fclose(fp);

	}

	vector<int> vIDs;

	//int argc = 7;

	//char tmpIter[128];
	//sprintf(tmpIter,"%d",iIter);

	//char tmpConvIter[128];
	//sprintf(tmpConvIter,"%d",iConvIter);

	//char tmpDamp[128];
	//sprintf(tmpDamp,"%f",dDamp);

	//char* argv[7] = {"jnk","__tmp__wclust_sims.txt","__tmp__wclust_prefs.txt","__tmp__wclust_ap_out.txt",tmpIter,tmpConvIter,tmpDamp};

	//apcluster_main(argc,argv,vIDs,pUp);
	
	apcluster_main(vSimsPrefs,vI,vK,iIter,iConvIter,dDamp,iRows,vIDs,pUp);

	int iV = 0;

	map<int,int> oMap;

	int idx = 0;

	typedef pair<int,int> int_pair;

	for(iV=0;iV<vIDs.size();iV++)
	{
		if(oMap.find(vIDs[iV])==oMap.end())
		{
			oMap.insert( int_pair(vIDs[iV],++idx ) );
		}
	}

	MY_STACK::iterator Index;	
	iV = 0;
	for (Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++)
	{	
		CVertex* verx = (CVertex*)*Index;

		//skip noise
		if(verx->GetNoise()) continue;

		std::map<int,int>::iterator IT = oMap.find(vIDs[iV]);

		verx->SetAPClust(IT->second);

		pUp->SetProgress("Saving AP results", 99.0 * iV / DataStack.m_VerxStack.size() );

		iV++;
	}

	map<int,int>::iterator IT = oMap.begin();
	int idxmax = 0;
	for(;IT!=oMap.end();IT++) if(IT->second>idxmax) idxmax=IT->second;

	m_pNumClusts[CLUST_AP] = idxmax;
}

void CCluster::ResistorInfoClust(CVerxStack& DataStack,int iClusts,const CUPDUPDATA* pUp)
{
	ProbInitFree oPIF(DataStack.m_VerxStack.size()+1);

	extern int pBins[4];
	const int iBins = pBins[2]; //initialized to 30, but may be modified by user
	int k = 0;

	int iDims=DataStack.GetAutoClusteringDimension(), iD=0;

	//used to skip Peak-V, Valley-V
	vector<int> vZeroes(iDims);
	for(k=4;k<=7;k++)   vZeroes[k]=1;
	for(k=12;k<=15;k++) vZeroes[k]=1;
	k=0;

	vector< prob_t > vInfo(iClusts+1);

	vector<int> vClustIDs(DataStack.NumNonNoiseVertices()) , vCounts(iClusts+1);

	randomassign(iClusts,vClustIDs.size(),&vClustIDs[0]);
	//make it 1-based indexing and init counts
	for(k=0;k<vClustIDs.size();k++)
	{
		vClustIDs[k]++;
		vCounts[vClustIDs[k]]++;
	}

	k=0;

	int iRows = 0, iCols = 0;
	int** pBinData = DataStack.GetVBinIDs<int>(true,iRows,iCols,iBins);
	AutoFree2D<int> AFData(pBinData);

	//fill distributions once
	vector< vector< Hist > > vDistribs;
	FillDistribs(DataStack,pBinData,*this,iBins,vDistribs,iClusts+1,vClustIDs);

	int iV = 0;

	//information gain
	vector<prob_t> vcInf(iClusts+1);

	m_pNumClusts[CLUST_INFO] = iClusts;

	vector< vector<prob_t> > vcInfInter(iClusts+1, vector<prob_t>(iClusts+1));
	int iC=1;

	const bool bUseCounts = true;

	//only need this for first time, since its recalculated for distributions who's
	//points change, when they change
	CalcUDDist(vDistribs,iClusts,vcInf,vcInfInter,vCounts,iRows,vZeroes,bUseCounts);

	int iIter = 0, iMaxIters = 500;
	int iPeriod = 10;
	
	vector<int> vSaved(vClustIDs.size());

	k = 0;

	char msg[1024];

	while(true)
	{
		if(pUp && pUp->ShouldTerminate()) break;

		if (iIter % iPeriod == 0) /* Save the current cluster assignments */
		{
			int i;
			for (i = 0; i < vClustIDs.size(); i++) vSaved[i] = vClustIDs[i];
			if (iPeriod < INT_MAX / 2) iPeriod *= 2;
		}

		++iIter;

		bool bMoved = false;

		int iV = 0 , iJnk = 0;

		for(iV=0;iV<vClustIDs.size();iV++,iJnk++)
		{
			if(pUp)
			{
				if(pUp->ShouldTerminate()) break;

				if(iJnk % 25 == 0)
				{
					sprintf(msg,"Iteration %d UD-info-max clustering checking vertex %d of %d",
						iIter,iV,vClustIDs.size());
					pUp->SetProgress(msg,99.0 * (prob_t) iV / (prob_t) vClustIDs.size());
				}
			}

			int iOrig = vClustIDs[iV] , iT = 1;

			if(vCounts[iOrig] <= 1) continue; //don't make any empty clusters

			vector<prob_t> vcInfTmp(iClusts+1), vGains(iClusts+1);
			vector< vector<prob_t> > vcInfInterTmp;
			
			//move vertex OUT of original distribution
			RemoveVertex(vDistribs[iOrig],pBinData[iV],iDims);

			prob_t dSumOrig = Sum(vcInf);
			//prob_t dAvgOrig = Avg(vcInf);
						
			for(iT=1;iT<=iClusts;iT++)
			{
				if(iT == iOrig) continue;

				//move vertex INTO temporary distribution
				AddVertex(vDistribs[iT],pBinData[iV],iDims);

				vector<int> vCountsTmp(vCounts);

				vCountsTmp[iOrig]--;
				vCountsTmp[iT]++;

				//calculate NET gain from moving vertex OUT of original distribution and INTO temp distribution
				CalcUDDist(vDistribs,iClusts,vcInfTmp,vcInfInterTmp,vCountsTmp,iRows,vZeroes,bUseCounts);
				
				vGains[iT] = Sum(vcInfTmp);//Sum(vcInfTmp) - dSumOrig;//vcInfTmp[iOrig] - vcInf[iOrig] + vcInfTmp[iT] - vcInf[iT]; //Sum(vcInfTmp);

				//move vertex OUT of temporary distribution
				RemoveVertex(vDistribs[iT],pBinData[iV],iDims);
			}

			//now see if moving the vertex to a different cluster had any net positive information gain
			int iMaxInd = iOrig; prob_t dMaxGain = dSumOrig;
			for(iT=1;iT<=iClusts;iT++)
			{
				if(vGains[iT] > dMaxGain)
				{
					dMaxGain = vGains[iT];
					iMaxInd = iT;
				}
			}

			if(iMaxInd != iOrig)
			{
				bMoved = true; //moved a point

				vClustIDs[iV] = iMaxInd;

				vCounts[iMaxInd]++;
				vCounts[iOrig]--;
				
				//move vertex INTO destination distribution
				AddVertex(vDistribs[iMaxInd],pBinData[iV],iDims);

				//recalculate information gain for the distributions
				//any of them could have been effected
				CalcUDDist(vDistribs,iClusts,vcInf,vcInfInter,vCounts,iRows,vZeroes,bUseCounts);
			}
			else //put it back into original distribution
			{
				AddVertex(vDistribs[iOrig],pBinData[iV],iDims);
			}
		}
		//didn't move any points, so we're finished
		if(!bMoved) break;

		int i = 0;
		for (i = 0; i < vClustIDs.size(); i++)
			if (vSaved[i]!=vClustIDs[i]) break;
		if (i==vClustIDs.size())
			break; /* Identical solution found; break out of this loop */
	}

	iV = 0;
	MY_STACK::iterator Index;
	for(Index=DataStack.m_VerxStack.begin();Index!=DataStack.m_VerxStack.end();Index++)
	{
		CVertex* vertex = (CVertex*)*Index;
		if(vertex->GetNoise())continue;
		vertex->SetInfoClust(vClustIDs[iV]);
		iV++;
	}
}

void CCluster::GetClusterInfo()
{
}

//////////////////////////////////////////////////////////////////////
// CProjection
void CProjection::Draw(CDC *pDC,CRect DrawWin,CFloatRect *whichValues,CPoint whichAxes,int Color,int style)
{
	int x1=0,y1=0,x2=0,y2=0,x0=0,y0=0;
	int Inverse=0;
	float fx0=0.0f,fy0=0.0f,fx1=0.0f,fy1=0.0f,fx2=0.0f,fy2=0.0f,pom=0.0f;
	
	MY_STACK::iterator Index;
	CPoint2D *MyPoint;
		
	if ((whichAxes.x==m_AxesX&&whichAxes.y==m_AxesY)||(whichAxes.x==m_AxesY&&whichAxes.y==m_AxesX))
	{
		Inverse=0;
		if (whichAxes.x==m_AxesY&&whichAxes.y==m_AxesX)
			Inverse=1;
		
		CPen newPen(PS_SOLID,1,m_PalStack->GetSColor(Color));//(COLORREF)0);
		CPen *pOldPen=pDC->SelectObject(&newPen);

		if (m_Pnt2DStack.begin()!=m_Pnt2DStack.end())
		{
			int DrawIt;
			for (Index=m_Pnt2DStack.begin();Index!=m_Pnt2DStack.end();Index++)
			{	
				if (Index == m_Pnt2DStack.end()-1)
				{
					MyPoint=(CPoint2D*) (*(Index));
				}
				else MyPoint=(CPoint2D*) (*Index);

				MyPoint->GetValue(&fx1,&fy1);
				if (Inverse) { pom=fx1; fx1=fy1; fy1=pom; }

				if (Index==m_Pnt2DStack.begin())
					{ x0=x1; y0=y1; fx0=fx1; fy0=fx1; }
			
				Index++;
				
				if (Index == m_Pnt2DStack.end())
				{
					MyPoint = (CPoint2D*) (*m_Pnt2DStack.begin());
				}
				else MyPoint=(CPoint2D*) (*Index);
				MyPoint->GetValue(&fx2,&fy2);
				if (Inverse) { pom=fx2; fx2=fy2; fy2=pom; }
				Index--;
/////
				DrawIt = 0;
				int State = 0;
				if ( fx2<whichValues->left || fx2>whichValues->right || fy2<whichValues->bottom || fy2>whichValues->top )
					State = 2;
				if ( fx1<whichValues->left || fx1>whichValues->right || fy1<whichValues->bottom || fy1>whichValues->top )
					State += 1;
				if (State>0)
				{
					if (State == 1)
					{	// swap xy1 <-> xy2
						float fxp,fyp;
						fxp = fx1; fyp = fy1;
						fx1 = fx2; fy1 = fy2;
						fx2 = fxp; fy2 = fyp;
					}
					
					// Parametrize
					// x = X1 + xv.t
					float fxv,fyv,t,tm;
					fxv = fx2 - fx1;
					fyv = fy2 - fy1;

					if (State < 3)
					{
						if (fyv == 0) //  - line
						{
							if (fx2 > whichValues->right)
								fx2 = whichValues->right;
							if (fx2 < whichValues->left)
								fx2 = whichValues->left;
							DrawIt = 1;
						}
						else if (fxv == 0) //  | line
						{
							if (fy2 > whichValues->top)
								fy2 = whichValues->top;
							if (fy2 < whichValues->bottom)
								fy2 = whichValues->bottom;
							DrawIt = 1;
						}
						else // not | or - line
						{
							if (fx2>fx1)
							{
								tm = (whichValues->right - fx1) / fxv;
								if (tm>0 && tm<=1)
								{
									t = tm;
								}
								else t=5;

								tm = (whichValues->bottom - fy1)  / fyv;
								if (tm>0 && tm<=1)
								{
									if (tm<t)
										t = tm;
								}

								tm = (whichValues->top - fy1) / fyv;
								if (tm>0 && tm<=1)
								{
									if (tm<t)
										t = tm;
								}
								fx2 = fx1 + fxv*t;
								fy2 = fy1 + fyv*t;
								DrawIt = 1;
							}
							else 
							{
								tm = (whichValues->left - fx1) / fxv;
								if (tm>0 && tm<=1)
								{
									t = tm;
								}
								else t=5;

								tm = (whichValues->bottom - fy1) / fyv;
								if (tm>0 && tm<=1)
								{
									if (tm<t)
										t = tm;
								}

								tm = (whichValues->top - fy1) / fyv;
								if (tm>0 && tm<=1)
								{
									if (tm<t)
										t = tm;
								}
								fx2 = fx1 + fxv*t;
								fy2 = fy1 + fyv*t;
								DrawIt = 1;
							}
						}
					}

					if (State == 3)
					{
						if (fyv == 0) //  - line
						{
							if (fy1 > whichValues->bottom &&  fy1 < whichValues->top && (fx1<whichValues->left && fx2>whichValues->right || fx2<whichValues->left && fx1>whichValues->right) )
							{
								fx1 = whichValues->left;
								fx2 = whichValues->right;
								DrawIt = 1;
							}
						}
						else if (fxv == 0) //  | line
						{
							if (fx1 > whichValues->left && fx1 < whichValues->right && (fy1<whichValues->bottom && fy2>whichValues->top || fy2<whichValues->bottom && fy1>whichValues->top))
							{
								fy1 = whichValues->bottom;
								fy2 = whichValues->top;
								DrawIt = 1;
							}
						}
						else // not | or - line
						{
							int NumFound = 0;
							float px,py;
							float tt[2];
							
							// left
							tm = (whichValues->left - fx1) / fxv;
							if (tm>0 && tm<=1) 
							{
								py = fy1 + fyv*tm;
								if (py >= whichValues->bottom && py <= whichValues->top)
								{
									tt[NumFound] = tm;
									NumFound ++;
								}
							}

							// bottom
							tm = (whichValues->bottom - fy1) / fyv;
							if (tm>0 && tm<=1) 
							{
								px = fx1 + fxv*tm;
								if (px > whichValues->left && px < whichValues->right)
								{
									tt[NumFound] = tm;
									NumFound++;
								}
							}

							// right
							tm = (whichValues->right - fx1) / fxv;
							if (tm>0 && tm<=1) 
							{
								py = fy1 + fyv*tm;
								if (py >= whichValues->bottom && py <= whichValues->top)
								{
									tt[NumFound] = tm;
									NumFound ++;
								}
							}

							// top
							tm = (whichValues->top - fy1) / fyv;
							if (tm>0 && tm<=1) 
							{
								px = fx1 + fxv*tm;
								if (px > whichValues->left && px < whichValues->right)
								{
									tt[NumFound] = tm;
									NumFound++;
								}
							}
						
							if (NumFound == 2)
							{
								fx2 = fx1 + fxv * tt[0];
								fy2 = fy1 + fyv * tt[0];
								fx1 = fx1 + fxv * tt[1];
								fy1 = fy1 + fyv * tt[1];
								DrawIt = 1;
							}
						}
					}
				}
				else DrawIt = 1; // State == 0

/////
				if (DrawIt)
				{
					x1=DrawWin.left+(fx1-whichValues->GetLeft())*(DrawWin.right-DrawWin.left)/whichValues->GetSizeX();
					y1=DrawWin.bottom+(fy1-whichValues->GetBottom())*(DrawWin.top-DrawWin.bottom)/whichValues->GetSizeY();
					
					x2=DrawWin.left+(fx2-whichValues->GetLeft())*(DrawWin.right-DrawWin.left)/whichValues->GetSizeX();
					y2=DrawWin.bottom+(fy2-whichValues->GetBottom())*(DrawWin.top-DrawWin.bottom)/whichValues->GetSizeY();
				
					pDC->MoveTo(x1,y1);
					pDC->LineTo(x2,y2);
				}

			}
		}
		pDC->SelectObject(pOldPen);
	}
}

int CProjection::AdaptAxes(int numb)
{
	//projections that contain the dimension specified by numb
	//are removed. otherwise, if the dimension is > numb
	//it is decremented by 1 , since the specified dimension
	//will be removed.
	if (m_AxesX == numb || m_AxesY == numb)
		return 1;
	
	if (m_AxesX > numb)
		m_AxesX -= 1;
	if (m_AxesY > numb)
		m_AxesY -= 1;
	
	return 0;
}

float CProjection::FindMinX()
{
	MY_STACK::iterator Index;
	float Value;
	CPoint2D *Pnt2D;
	Pnt2D = (CPoint2D*) *m_Pnt2DStack.begin();
	Value = Pnt2D->m_X;
	for (Index=m_Pnt2DStack.begin();Index!=m_Pnt2DStack.end();++Index)
	{
		Pnt2D = (CPoint2D*) *Index;
		if ( Pnt2D->m_X < Value )
			Value = Pnt2D->m_X;
	}
	return Value;
}

float CProjection::FindMaxX()
{
	MY_STACK::iterator Index;
	float Value;
	CPoint2D *Pnt2D;
	Pnt2D = (CPoint2D*) *m_Pnt2DStack.begin();
	Value = Pnt2D->m_X;
	for (Index=m_Pnt2DStack.begin();Index!=m_Pnt2DStack.end();++Index)
	{
		Pnt2D = (CPoint2D*) *Index;
		if ( Pnt2D->m_X > Value )
			Value = Pnt2D->m_X;
	}
	return Value;	
}

float CProjection::FindMinY()
{
	MY_STACK::iterator Index;
	float Value;
	CPoint2D *Pnt2D;
	Pnt2D = (CPoint2D*) *m_Pnt2DStack.begin();
	Value = Pnt2D->m_Y;
	for (Index=m_Pnt2DStack.begin();Index!=m_Pnt2DStack.end();++Index)
	{
		Pnt2D = (CPoint2D*) *Index;
		if ( Pnt2D->m_Y < Value )
			Value = Pnt2D->m_Y;
	}
	return Value;
}

float CProjection::FindMaxY()
{
	MY_STACK::iterator Index;
	float Value;
	CPoint2D *Pnt2D;
	Pnt2D = (CPoint2D*) *m_Pnt2DStack.begin();
	Value = Pnt2D->m_Y;
	for (Index=m_Pnt2DStack.begin();Index!=m_Pnt2DStack.end();++Index)
	{
		Pnt2D = (CPoint2D*) *Index;
		if ( Pnt2D->m_Y > Value )
			Value = Pnt2D->m_Y;
	}
	return Value;
}

void CProjection::SetEmpty()
{
	MY_STACK::iterator Index;
	CPoint2D *pnt2D;
	for (Index=m_Pnt2DStack.begin();Index!=m_Pnt2DStack.end();Index++)
	{
		pnt2D = (CPoint2D*)*Index;
		delete pnt2D;
	}
	m_Pnt2DStack.clear();
	
	CVect2D *v2D;
	for (Index=m_Vect2DStack.begin();Index!=m_Vect2DStack.end();Index++)
	{
		v2D = (CVect2D*)*Index;
		delete v2D;
	}
	m_Vect2DStack.clear();
}

int CProjection::IsIn(CVertex *i_verx)
{
	MY_STACK::iterator Index;
	CVect2D *vect;
	int m_Is=1;
	
	Index=m_Vect2DStack.begin();
	if (Index==m_Vect2DStack.end())
		m_Is=0;
	while (Index!=(m_Vect2DStack.end())&&m_Is)
	{
		vect = (CVect2D*) *Index;
		m_Is=vect->IsIn(i_verx->GetValue(m_AxesX),i_verx->GetValue(m_AxesY));
		Index++;
	}
	if (m_Is)
	{
		return 1;
	}
	else return 0;
}

int CProjection::StoreData(CFile *file)
{
	MY_STACK::iterator Index;

	file->Write("P",1);
	file->Write(&m_AxesX,sizeof(m_AxesX));
	file->Write(&m_AxesY,sizeof(m_AxesY));
	file->Write(&m_Count,sizeof(m_Count));

	CPoint2D *pnt2D;
	for (Index=m_Pnt2DStack.begin();Index!=m_Pnt2DStack.end();Index++)
	{
		pnt2D = (CPoint2D*) *Index;
		pnt2D->StoreData(file);
	}

	CVect2D *vect2D;
	for (Index=m_Vect2DStack.begin();Index!=m_Vect2DStack.end();Index++)
	{
		vect2D = (CVect2D*) *Index;
		vect2D->StoreData(file);
	}
	return 0;
}

void CProjection::ConvertAxes(char Version,int iCurDims)
{
	if (Version == 10)
	{
		if (m_AxesX <= 4)
			m_AxesX *= 4;
		else m_AxesX = (m_AxesX - 5)*4 + 2;

		if (m_AxesY <= 4)
			m_AxesY *= 4;
		else m_AxesY = (m_AxesY - 5)*4 + 2;

		Version = 11;
	}
	else if (Version == 11)
	{
		if (m_AxesX <= 16)
		{
			int divi = (m_AxesX-1) / 4;
			int rest = (m_AxesX-1) % 4;
			m_AxesX = 4*rest + divi+1;
		}
		if (m_AxesY <= 16)
		{
			int divi = (m_AxesY-1) / 4;
			int rest = (m_AxesY-1) % 4;
			m_AxesY = 4*rest + divi+1;
		}

		Version = 12;
	}
	else if(Version == 13)
	{
		if(m_AxesX > 16)
		{
			m_AxesX = (m_AxesX - 17) + iCurDims;
		}
		if(m_AxesY > 16)
		{
			m_AxesY = (m_AxesY - 17) + iCurDims;
		}
	}
	else if(Version >= 14)
	{
		//no pca and added by user dimension
		//or pca 
		//or pca and added by user dimension
		/*if(iStartPCADim > 0)
		{
			if(m_AxesX > iEndPCADim)
			{
			}
		}
		else
		{
			if(m_AxesX > 20)
			{
				m_AxesX = (m_AxesX - 21) + iCurDims;
			}
			if(m_AxesY > 20)
			{
			}
		}*/
	}
	
	// if new version, here will be if for them

}

int CProjection::LoadData(CFile *file, char Version,int iCurDims)
{
	int errorLoad, NumbLoad, Count;
	NumbLoad=file->Read(&m_AxesX,sizeof(m_AxesX));
	if (NumbLoad!=sizeof(m_AxesX))
		return 8;
	NumbLoad=file->Read(&m_AxesY,sizeof(m_AxesY));
	if (NumbLoad!=sizeof(m_AxesY))
		return 8;
	NumbLoad=file->Read(&Count,sizeof(Count));
	if (NumbLoad!=sizeof(Count))
		return 8;

	ConvertAxes(Version,iCurDims);
	
	CPoint2D *pnt2D;
	for (int j=0;j<Count;++j)
	{
		pnt2D = new CPoint2D();
		AddPnt2D(pnt2D);
		errorLoad=pnt2D->LoadData(file);
		if (errorLoad)
			return errorLoad;
	}

	CVect2D *vect2D;
	int k;
	for (k=0;k<Count;++k)
	{
		vect2D = new CVect2D();
		AddVect2D(vect2D);
		errorLoad=vect2D->LoadData(file);
		if (errorLoad)
			return errorLoad;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CBoundary
void CBoundary::SetEmpty()
{
	MY_STACK::iterator Index;
	CProjection *proj;
	for (Index=m_ProjStack.begin();Index!=m_ProjStack.end();Index++)
	{
		proj = (CProjection*)*Index;
		proj->SetEmpty();
		delete proj;
	}
	m_ProjStack.clear();
	m_Count=0;
}

void CBoundary::AdaptProj(int numb)
{
	MY_STACK::iterator indexProj;
	int pomInd;

	CProjection *Proj;
	indexProj = m_ProjStack.begin();
	//this loop searches for projections that contain
	//the dimension specified by numb. if they do, they
	//are removed. otherwise, if they have dimensions > numb
	//they are decremented by 1 , since the specified dimension
	//will be removed.
	while (indexProj != m_ProjStack.end())
	{
		Proj = (CProjection*) *indexProj;
		if (Proj->AdaptAxes(numb)) 
		{
			pomInd = indexProj - m_ProjStack.begin();

			Proj->SetEmpty();
			delete Proj;
			m_Count -= 1;

			m_ProjStack.erase(indexProj);

			indexProj = m_ProjStack.begin() + pomInd;
		}
		else 
		{
			indexProj++;
		}
	}
}

void CBoundary::Draw(CDC *pDC,CRect DrawWin,CFloatRect *whichValues,CPoint whichAxes,unsigned char *Clust,int style)
{
	MY_STACK::iterator Index;
	if ( ((*(Clust+m_Numb)) & 2)  !=0  )
	{		
		for (Index=m_ProjStack.begin();Index!=m_ProjStack.end();Index++)
		{
			((CProjection*)*Index)->Draw(pDC,DrawWin,whichValues,whichAxes,m_Numb,style);
		}
	}
}

int CBoundary::FindMinXY(int which, float *Value)
{
	MY_STACK::iterator Index;
	CProjection *MyProj;
	
	for (Index=m_ProjStack.begin(); Index!=m_ProjStack.end(); ++Index)
	{
		MyProj = (CProjection*) *Index;
		if (MyProj->m_AxesX==which)
		{
			*Value = MyProj->FindMinX();
			return 1;
		}
		if (MyProj->m_AxesY==which)
		{
			*Value = MyProj->FindMinY();
			return 1;
		}
	}
	return 0;
}

int CBoundary::FindMaxXY(int which, float *Value)
{
	MY_STACK::iterator Index;
	CProjection *MyProj;
	
	for (Index=m_ProjStack.begin(); Index!=m_ProjStack.end(); ++Index)
	{
		MyProj = (CProjection*) *Index;
		if (MyProj->m_AxesX==which)
		{
			*Value = MyProj->FindMaxX();
			return 1;
		}
		if (MyProj->m_AxesY==which)
		{
			*Value = MyProj->FindMaxY();
			return 1;
		}
	}
	return 0;
}
/*
void CBoundary::Clustering(CVerxStack *i_VerxStack)
{
	MY_STACK::iterator Index;
	MY_STACK::iterator PntsIndex;
	int m_isIn;
	CProjection *m_proj;
	CVertex *m_vx;
	
	for (PntsIndex=i_VerxStack->m_VerxStack.begin();PntsIndex!=i_VerxStack->m_VerxStack.end();PntsIndex++)
	{
		m_isIn=1;
		m_vx = (CVertex*) *PntsIndex;
		
		Index=m_ProjStack.begin();
		if (Index==m_ProjStack.end())
			m_isIn=0;
		while (Index != m_ProjStack.end() && m_isIn)
		{
			m_proj=(CProjection*) *Index;
			m_isIn=m_proj->IsIn(m_vx);
			Index++;
		}

		if (m_isIn)
		{	
			int toStore;
			toStore = m_Numb + 0x10000 * m_Count;
			m_vx->AddClust(toStore);	
//			m_vx->AddClust(m_Numb );
		}
	}
}
*/

bool CBoundary::IsIn(CVertex* Pnt)
{	
	bool bIsIn=true;
		
	MY_STACK::iterator Index=m_ProjStack.begin();

	if (Index==m_ProjStack.end())
		bIsIn=false;

	while ( Index != m_ProjStack.end() && bIsIn)
	{
		CProjection* proj=(CProjection*) *Index;
		bIsIn=proj->IsIn(Pnt);
		Index++;
	}

	return bIsIn;
}

void CBoundary::Clustering(CVertex *Pnt,bool NoiseOn)
{
	MY_STACK::iterator Index;
	int m_isIn;
	CProjection *m_proj;
	
	m_isIn=1;
		
	Index=m_ProjStack.begin();
	if (Index==m_ProjStack.end())
		m_isIn=0;
	while ( Index != m_ProjStack.end() && m_isIn)
	{
		m_proj=(CProjection*) *Index;
		m_isIn=m_proj->IsIn(Pnt);
		Index++;
	}

	if (m_isIn)
	{	
		//Pnt->SetNoise(0);		
		//what is this doing??
		int toStore = m_Numb + 0x10000 * m_Count;
		Pnt->AddClust(toStore);	
		//if(NoiseOn)Pnt->SetNoise(true); else Pnt->SetNoise(false);
		if(this->m_bNoise) Pnt->SetNoise(true);
	}
}


int CBoundary::StoreData(CFile *file)
{
	MY_STACK::iterator Index;

	file->Write("B",1);
	
	if(this->m_bNoise)
	{	//if it's a noise cluster write indicator to .cl file
		int iTmp = -123456;
		file->Write(&iTmp,sizeof(iTmp));
	}
	
	file->Write(&m_Count,sizeof(m_Count));

	CProjection *proj;
	for (Index=m_ProjStack.begin();Index!=m_ProjStack.end();Index++)
	{
		proj = (CProjection*) *Index;
		proj->StoreData(file);
	}
	return 0;
}

int CBoundary::LoadData(CFile *file, char Version, int iCurDims)
{
	int NumbLoad,errorLoad=0, Count;

	NumbLoad=file->Read(&Count,sizeof(Count));
	if (NumbLoad!=sizeof(Count))
		return 6;

	if(Count == -123456)
	{	//check if it's a noise cluster
		this->m_bNoise = true;
		NumbLoad=file->Read(&Count,sizeof(Count));
		if (NumbLoad!=sizeof(Count))
			return 6;
	}

	if (Count>0)
	{
		char identif;
		CProjection *proj;
		for (int i=1;i<=Count;i++)
		{
			proj = new CProjection(m_PalStack);
			AddProj(proj);
			
			NumbLoad=file->Read(&identif,1);
			if (NumbLoad!=1)
				return 6;

			if (identif=='P')
				errorLoad=proj->LoadData(file, Version, iCurDims);
			else return 7;
			if (errorLoad)
				return errorLoad;
		}
	}
	else m_Count=0;
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CPoint2D
int CPoint2D::StoreData(CFile *file)
{
	file->Write("N",1);
	file->Write(&m_X,sizeof(m_X));
	file->Write(&m_Y,sizeof(m_Y));
	return 0;
}

int CPoint2D::LoadData(CFile *file)
{
	char pom;
	int NumRead;
	NumRead=file->Read(&pom,1);
	if (NumRead!=1)
		return 12;
	if (pom!='N')
		return 13;
	NumRead=file->Read(&m_X,sizeof(m_X));
	if (NumRead!=sizeof(m_X))
		return 12;
	NumRead=file->Read(&m_Y,sizeof(m_Y));
	if (NumRead!=sizeof(m_Y))
		return 12;
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CVect2D
int CVect2D::StoreData(CFile *file)
{
	file->Write("V",1);
	file->Write(&m_X,sizeof(m_X));
	file->Write(&m_Y,sizeof(m_Y));
	file->Write(&m_b,sizeof(m_b));
	return 0;
}

int CVect2D::LoadData(CFile *file)
{
	int NumRead;
	char pom;
	NumRead=file->Read(&pom,1);
	if (NumRead!=1)
		return 10;
	if (pom!='V')
		return 11;
	NumRead=file->Read(&m_X,sizeof(m_X));
	if (NumRead!=sizeof(m_X))
		return 10;
	NumRead=file->Read(&m_Y,sizeof(m_Y));
	if (NumRead!=sizeof(m_Y))
		return 10;
	NumRead=file->Read(&m_b,sizeof(m_b));
	if (NumRead!=sizeof(m_b))
		return 10;
	return 0;
}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
