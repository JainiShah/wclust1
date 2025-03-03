// $Id: InfoT.cpp,v 1.4 2008/04/11 22:27:40 samn Exp $ 
#include "stdafx.h"
#include "InfoT.h"

//#define LOG_TEST 1

MTimer oMT("getnnrsq btest=true");
MTimer oMF("getnnrsq btest=false");

bool NeighborClustCount(Neighbor** vNeighbors,vector<int>& vClustIDs,vector<ClusterInfo>& vPrct,int iNNToFind,int iClusts)
{
	int iSz = vClustIDs.size();
	if(vPrct.size()!=iClusts+1)
		return false;
	int i;
	vector<int> vCounts(iClusts+1);
	for(i=1;i<=iClusts;i++)
		vCounts[i]=count(vClustIDs.begin(),vClustIDs.end(),i);
	vector< vector<prob_t> > vprct(iClusts+1);
	for(i=1;i<=iClusts;i++)
		vprct[i]=vector<prob_t>(vCounts[i],0.0);
	vector<int> vIndex(iClusts+1,0);
	for(i=0;i<iSz;i++)
	{
		int j, iCount = 0, iClust = vClustIDs[i];
		if(!iClust)continue;//skip background vectors
		int idx = vIndex[iClust]++;
		for(j=0;j<iNNToFind;j++)
		{
			if(vClustIDs[vNeighbors[i][j].m_id]==iClust)
				vprct[iClust][idx]+=1.0f;
		}
		vprct[iClust][idx] /= iNNToFind;
	}
	for(i=1;i<=iClusts;i++)
		vPrct[i].m_fPrctKNNInClust=Avg(vprct[i]);
	return true;
}

prob_t FastKLDivPQ(KDTreeHist& p,KDTreeHist& q,Neighbor** vNeighbors,int iClustIDP,int iClustIDQ,vector<int>& vClustIDs,int iNNToFind,vector<int>& vNCount)
{
	int iFastP = 0, iFastQ = 0, iSlowP = 0 , iSlowQ = 0;

#ifdef LOG_TEST
	double kldiv = 1.0;
	double kldivQ=1.0,kldivP=1.0;
#else
	prob_t kldiv = 0.0;
#endif
	int isz = vClustIDs.size() , iV = 0, iT = 0, iNN = 0, iLessP = 0, iLessQ = 0;

	for(iV=0;iV<isz;iV++)
	{
		if(vClustIDs[iV]!=iClustIDP)	//skip vectors not in cluster p
			continue;

		prob_t dDistP = 0.0 , dDistQ = 0.0; // squared distances
		
		Neighbor* vnn = vNeighbors[iV]; //these are the neighbors of an element in cluster p
		int nsz = vNCount[iV]; //iNNToFind;//vnn.size();
		bool bFoundP = false, bFoundQ = false;
		for(iNN=0;iNN<nsz;iNN++)
		{	Neighbor& oN = vnn[iNN];
			if(!bFoundP && vClustIDs[oN.m_id]==iClustIDP)
			{	if(oN.m_dist>0.0)
				{	//found a different neighbor in the same cluster p
					dDistP = oN.m_dist;
					iFastP++;
					bFoundP = true;
				}
			}
			else if(!bFoundQ && vClustIDs[oN.m_id]==iClustIDQ)
			{	//found a neighbor in the other cluster q
				dDistQ = oN.m_dist;
				iFastQ++;
				bFoundQ = true;
			}
			if(bFoundP && bFoundQ) break;	//found neighbors so break
		}
		if(!bFoundP)
		{	//use slow method of searching KD-tree
			dDistP = p.GetNearestRadiusSQ(iT,false);
//#ifdef _DEBUG
			float dDistP2 = p.GetNearestRadiusSQ(iT,true);
			if(dDistP!=dDistP2)
			{
				Write2Log("not eq %.6f %.6f",dDistP,dDistP2);
			}
//#endif
			iSlowP++;
		}
		if(!bFoundQ)
		{	//use slow method of searching KD-tree
			dDistQ = q.GetNearestRadiusSQ(p[iT],true,false);//true==alow zero distance,since diff cluster,can have same exact point with 0 dist
//#ifdef _DEBUG
			float dDistQ2 = q.GetNearestRadiusSQ(p[iT],true,true);
			if(dDistQ2!=dDistQ)
			{
				Write2Log("not eq %.6f %.6f",dDistQ,dDistQ2);
			}
//#endif
			iSlowQ++;
		}

		if(dDistP>0.0 && dDistQ>0.0)
		{	//avoid exceptions of log(0)
			//if(dDistP<dDistQ)iLessP++; else iLessQ++;
#ifdef LOG_TEST
			//kldiv *= dDistQ / dDistP;
	//		kldivQ *= dDistQ;
	//		kldivP *= dDistP;
			kldiv += log( dDistQ / dDistP );
#else
			kldiv += log2( dDistQ / dDistP ) / 2.0;
#endif
		}
		
		iT++;	//increment index into cluster p's KD-tree
	}
	//finish the calculation
#ifdef LOG_TEST
	//kldiv = log2( kldiv ) / 2.0;
	//kldiv = log2( kldivQ / kldivP ) / 2.0;
	kldiv /= ( log(2.0) * 2.0 );
#endif
	kldiv *= p.NumDims() / ((prob_t) p.NumElems() );
	kldiv += log2( (prob_t)q.NumElems() / (p.NumElems()-1.0 ) );
	//write some stats
	Write2Log("FastKLDivPQ:kldiv=%.4f iSlow=%d iSlowP=%d iSlowQ=%d iFast=%d iFastP=%d iFastQ=%d",kldiv,iSlowP+iSlowQ,iSlowP,iSlowQ,iFastP+iFastQ,iFastP,iFastQ);
	
	return kldiv;
}

prob_t FastKLDivPNOTP(KDTreeHist& p,KDTreeHist& notp,Neighbor** vNeighbors,int iClustIDP,vector<int>& vClustIDs,int iNNToFind,vector<int>& vNCount)
{
	int iFastP = 0, iFastNotP = 0, iSlowP = 0 , iSlowNotP = 0;

#ifdef LOG_TEST
	double kldiv = 1.0;
	double kldivP=1.0,kldivNOTP=1.0;
#else
	prob_t kldiv = 0.0;
#endif
	int isz = vClustIDs.size() , iV = 0, iT = 0, iNN = 0, iLessP = 0, iLessNotP = 0;

	for(iV=0;iV<isz;iV++)
	{
		if(vClustIDs[iV]!=iClustIDP)	//skip vectors not in cluster p
			continue;

		prob_t dDistP = 0.0 , dDistNotP = 0.0; // squared distances
		
		Neighbor* vnn = vNeighbors[iV]; //these are the neighbors of an element in cluster p
		int nsz = vNCount[iV]; //iNNToFind;//vnn.size();
		bool bFoundP = false, bFoundNotP = false;
		for(iNN=0;iNN<nsz;iNN++)
		{	Neighbor& oN = vnn[iNN];
			if(!bFoundP && vClustIDs[oN.m_id]==iClustIDP)
			{	if(oN.m_dist>0.0)
				{	//found a different neighbor in the same cluster p
					dDistP = oN.m_dist;
					iFastP++;
					bFoundP = true;
				}
			}
			else if(!bFoundNotP && vClustIDs[oN.m_id]!=iClustIDP)
			{	//found a neighbor in the other cluster q
				dDistNotP = oN.m_dist;
				iFastNotP++;
				bFoundNotP = true;
			}
			if(bFoundP && bFoundNotP) break;	//found neighbors so break
		}
		if(!bFoundP)
		{	//use slow method of searching KD-tree
			dDistP = p.GetNearestRadiusSQ(iT,false);
//#ifdef _DEBUG
			float dDistP2 = p.GetNearestRadiusSQ(iT,true);
			if(dDistP!=dDistP2)
			{
				Write2Log("not eq %.6f %.6f",dDistP,dDistP2);
			}
//#endif
			iSlowP++;
		}
		if(!bFoundNotP)
		{	//use slow method of searching KD-tree
			dDistNotP = notp.GetNearestRadiusSQ(p[iT],true,false);//true==alow zero distance,since diff cluster,can have same exact point with 0 dist
//#ifdef _DEBUG
			float dDistNotP2 = notp.GetNearestRadiusSQ(p[iT],true,true);
			if(dDistNotP!=dDistNotP2)
			{
				Write2Log("not eq %.6f %.6f",dDistNotP,dDistNotP2);
			}
//#endif
			iSlowNotP++;
		}

		if(dDistP>0.0 && dDistNotP>0.0)
		{	//avoid exceptions of log(0)
			//if(dDistP<dDistNotP)iLessP++; else iLessNotP++;
#ifdef LOG_TEST
			//kldiv *= dDistNotP / dDistP;
			//kldivP *= dDistP;
			//kldivNOTP *= dDistNotP;
			kldiv += log ( dDistNotP / dDistP );
#else
			kldiv += log2( dDistNotP / dDistP ) / 2.0;
#endif
		}
		
		iT++;	//increment index into cluster p's KD-tree
	}
	//finish the calculation
#ifdef LOG_TEST
	//kldiv = log2( kldiv ) / 2.0;
	//kldiv = log2( kldivNOTP / kldivP ) / 2.0;
	kldiv /= ( log(2.0) * 2.0 );
#endif
	kldiv *= p.NumDims() / ((prob_t) p.NumElems() );
	kldiv += log2( (prob_t)notp.NumElems() / (p.NumElems()-1.0 ) );
	//write some stats
	Write2Log("FastKLDivPNOTP:kldiv=%.4f iSlow=%d iSlowP=%d iSlowNotP=%d iFast=%d iFastP=%d iFastNotP=%d",kldiv,iSlowP+iSlowNotP,iSlowP,iSlowNotP,iFastP+iFastNotP,iFastP,iFastNotP);
	
	return kldiv;
}

//notp is all points not in p
//this version gets the distance from notp to p
prob_t FastKLDivNOTPP(KDTreeHist& p,KDTreeHist& notp,Neighbor** vNeighbors,int iClustIDP,vector<int>& vClustIDs,int iNNToFind,vector<int>& vNCount)
{
	int iFastP = 0, iFastNP = 0, iSlowP = 0, iSlowNP = 0;

#ifdef LOG_TEST
	double kldiv = 1.0;
	double kldivNOTP=1.0,kldivP=1.0;
#else
	prob_t kldiv = 0.0;
#endif
	int isz = vClustIDs.size() , iV = 0, iT = 0, iNN = 0, iLessP = 0, iLessNotP = 0;

	for(iV=0;iV<isz;iV++)
	{
		if(vClustIDs[iV]==iClustIDP) //skip all points in p
			continue;

		prob_t dDistP = 0.0 , dDistNotP = 0.0;
		
		Neighbor* vnn = vNeighbors[iV];
		int nsz = vNCount[iV]; //iNNToFind;
		bool bFoundP = false, bFoundNotP = false;
		for(iNN=0;iNN<nsz;iNN++)
		{	Neighbor& oN = vnn[iNN];
			if(!bFoundP && vClustIDs[oN.m_id]==iClustIDP)
			{
				dDistP = oN.m_dist;
				iFastP++;
				bFoundP = true;
			}
			else if(!bFoundNotP && vClustIDs[oN.m_id]!=iClustIDP)
			{
				if(oN.m_dist>0.0)
				{
					dDistNotP = oN.m_dist;
					iFastNP++;
					bFoundNotP = true;
				}
			}
			if(bFoundP && bFoundNotP) break;
		}
		if(!bFoundP)
		{
			dDistP = p.GetNearestRadiusSQ(notp[iT],true,false);
//#ifdef _DEBUG
			float dDistP2 = p.GetNearestRadiusSQ(notp[iT],true,true);
			if(dDistP!=dDistP2)
			{
				Write2Log("not eq %.6f %.6f",dDistP,dDistP2);
			}
//#endif
			iSlowP++;
		}
		if(!bFoundNotP)
		{
			dDistNotP = notp.GetNearestRadiusSQ(iT,false);
//#ifdef _DEBUG
			float dDistNotP2 = notp.GetNearestRadiusSQ(iT,true);
			if(dDistNotP!=dDistNotP2)
			{
				Write2Log("not eq %.6f %.6f",dDistNotP,dDistNotP2);
			}
//#endif
			iSlowNP++;
		}

		if(dDistP>0.0 && dDistNotP>0.0)
		{
			//if(dDistNotP<dDistP)iLessNotP++; else iLessP++;
#ifdef LOG_TEST
			//kldiv *= dDistP / dDistNotP;
			//kldivP *= dDistP;
			//kldivNOTP *= dDistNotP;
			kldiv += log ( dDistP / dDistNotP );
#else
			kldiv += log2( dDistP / dDistNotP ) / 2.0;
#endif
		}
		
		iT++;
	}
#ifdef LOG_TEST
	//kldiv = log2( kldiv ) / 2.0;
	//kldiv = log2( kldivP / kldivNOTP ) / 2.0;
	kldiv /= ( log(2.0) * 2.0 );
#endif
	kldiv *= notp.NumDims() / ((prob_t) notp.NumElems() );
	kldiv += log2( (prob_t)p.NumElems() / (notp.NumElems()-1.0 ) );

	Write2Log("FastKLDivNOTPP:kldiv=%.4f iSlow=%d iSlowP=%d iSlowNP=%d iFast=%d iFastP=%d iFastNP=%d",kldiv,iSlowP+iSlowNP,iSlowP,iSlowNP,iFastP+iFastNP,iFastP,iFastNP);
	
	return kldiv;
}

//symmetric kldiv 
//(dpq * dqp ) / (dpq + dqp)
//dpq == distance from p to q
//dqp == distance from q to p
//both q and p are actual clusters with valid IDs!!
prob_t FastKLDivSymPQ(KDTreeHist& p,KDTreeHist& q,Neighbor** vNeighbors,int iClustIDP,int iClustIDQ,vector<int>& vClustIDs,int iNNToFind,vector<int>& vNCount)
{
	prob_t dpq = FastKLDivPQ(p,q,vNeighbors,iClustIDP,iClustIDQ,vClustIDs,iNNToFind,vNCount), //dist from p to q
		   dqp = FastKLDivPQ(q,p,vNeighbors,iClustIDQ,iClustIDP,vClustIDs,iNNToFind,vNCount); //dist from q to p
	if(!dpq && !dqp) return 0.0;
	return dpq*dqp / (dpq+dqp);
}

//symmetric kldiv
//(dpnotp * dnotpp) / (dpnotp + dnotpp)
//dpnotp == distance from p to not p
//dnotpp == distance from not p to p
//p == actual cluster with valid ID
//notp == complement of p (all vectors not in p), without an actual ID 
prob_t FastKLDivSymPNOTP(KDTreeHist& p,KDTreeHist& notp,Neighbor** pNeighbors,int iClustIDP,vector<int>& vClustIDs,int iNNToFind,vector<int>& vNCount)
{
	prob_t dpq = FastKLDivPNOTP(p,notp,pNeighbors,iClustIDP,vClustIDs,iNNToFind,vNCount),  //dist from p to notp
		   dqp = FastKLDivNOTPP(p,notp,pNeighbors,iClustIDP,vClustIDs,iNNToFind,vNCount); //dist from notp to p
	if(!dpq && !dqp) return 0.0;
	return dpq*dqp / (dpq+dqp);
}

//computes kldiv between each pair of actual clusters and adds the min to CCluster::m_vInfo
bool InterClustKLD(CCluster& Clusters,vector<KDTreeHist>& vDistribs,vector<int>& vClustIDs,int iClusts,bool bFast,Neighbor** vnn,int WhichDraw,const CUPDUPDATA* pUp,int iNNToFind,vector<int>& vNCount)
{	//kldiv cluster to other clusters
	int iC1 = 0, iC2 = 0, iTot = ((iClusts-1)*(iClusts-1)+iClusts-1)/2, iCurr = 0;
	try
	{	char msg[1024]={0};
		vector< vector<prob_t> > vcInfInter(iClusts+1, vector<prob_t>(iClusts+1));		
		Write2Log("Calculating inter-cluster KLDiv");
		prob_t kldiv = 0.0f;
		for(iC1=1;iC1<=iClusts && !pUp->ShouldTerminate();iC1++)
		{	for(iC2=iC1+1;iC2<=iClusts && !pUp->ShouldTerminate();iC2++,iCurr++)
			{	sprintf(msg,"Calculating kldiv btwn clust %d and %d",iC1,iC2);
				pUp->SetProgress(msg,100*(iCurr/static_cast<double>(iTot)));
				Write2Log(msg);
				if(bFast)
					vcInfInter[iC1][iC2]=vcInfInter[iC2][iC1]=FastKLDivSymPQ(vDistribs[iC1],vDistribs[iC2],vnn,iC1,iC2,vClustIDs,iNNToFind,vNCount);
				else
					vcInfInter[iC1][iC2]=vcInfInter[iC2][iC1]=KLDivSym(vDistribs[iC1],vDistribs[iC2]);
				Write2Log("sym. kldiv from %d to %d = %.4f",iC1,iC2,vcInfInter[iC1][iC2]);
			}
		}
		if(!pUp->ShouldTerminate()){ for(iC1=1;iC1<=iClusts;iC1++)
		{	prob_t min_int = INF; //*INF;
			int min_ind = 0;
			for(iC2=1;iC2<=iClusts;iC2++)
			{	if(iC1==iC2)continue;
				prob_t tmpK = vcInfInter[iC1][iC2];
				if(tmpK<min_int)
				{	min_int=tmpK; 
					min_ind=iC2; 
				}
			}
			Clusters.m_vInfo[WhichDraw][iC1].m_fInterClustGain = min_int;
			Clusters.m_vInfo[WhichDraw][iC1].m_iClosestID = min_ind;
			Write2Log("Nearest kldiv from clust %d to %d is %.6f",iC1,min_ind,min_int);
		}
		CString strTab("\ninter clust kldiv table\n") , strTmp;//write inter-cluster kldiv table to log for inspection...
		for(iC1=1;iC1<=iClusts;iC1++)
		{	for(iC2=1;iC2<=iClusts;iC2++)
			{	strTmp.Format("%.6f\t",vcInfInter[iC1][iC2]);
				strTab += strTmp;
			}
			strTab += "\n";			
		}
		Write2Log(strTab);
		}
	} 
	catch(...)
	{	Write2Log("Exception in InterClustKLD!!! iC1=%d iC2=%d iClusts=%d",iC1,iC2,iClusts);
		return false;
	}
	return true;
}

//gets KLDiv using approximate probabilities
//P(j) = (nj+0.5)/(N+0.5jmax)
//n1+n2+...+nmax=N
//the p distribution's counts are first subtracted
//from the q distribution so that the function will
//calculate the "exclusive" information gain
//p is "partial" distribution
//q is "full" distribution
prob_t KLDivApproxExclusiveProb(Hist& p,Hist& q)
{
	if(p.NumBins() != q.NumBins()) return -1.0;

	prob_t d = 0.0;

	int i = 0, iBins = p.NumBins();

	prob_t eps = 1e-100;

	prob_t pelems = p.NumElems();
	prob_t qelems = q.NumElems();

	for(i=0;i<iBins;i++)
	{
		prob_t pcount = p[i];
		prob_t qcount = q[i];

		//approximate probability of bin i in distribution p
		prob_t pp = (pcount+0.5) / ( pelems + 0.5*iBins);

		//approximate probability of bin i in distribution q
		//must subtract pcount elements from numerator & denominator of probability
		//approximation so as to maintain "exclusivity" of p distribution
//		prob_t qp = (qcount - pcount + 0.5) / ( qelems - pcount + 0.5*iBins);
		prob_t qp = (qcount - pcount + 0.5) / ( qelems - pelems + 0.5*iBins);

		if(pp<=eps) continue;

		if(qp<=eps) continue;

		d += pp * ( log2(pp) - log2(qp) );
	}

	return d;
}

prob_t KLDiv(KDTreeHist& p,KDTreeHist& q)
{	
	int i=0;
#ifdef DO_TIMING
	char sMsg[1024];
	sprintf(sMsg,"KLDiv szp=%d, szq=%d, dims=%d",p.NumElems(),q.NumElems(),p.NumDims());
	ScopedTimer S(sMsg);
#endif
	
	try	//just in case something goes wrong from invalid type of data passed in
	{
		//make sure we can do calculation with div by zero and need same # of dimensions 
		//in each distribution
		if(p.NumElems() < 2 || q.NumElems()<1 || p.NumDims()!=q.NumDims()) return 0.0;

		int iLessP = 0, iLessQ = 0;

		prob_t dpq = 0.0;

		const prob_t eps = 0.0;

		int isz = p.NumElems();

		i = 0;
		for(i=0;i<isz;i++)
		{
			prob_t distp = p.GetNearestRadiusSQ(i,false);
//#ifdef _DEBUG
			prob_t distp2 = p.GetNearestRadiusSQ(i,true);
			if(distp!=distp2)
			{
				Write2Log("not eq %.6f %.6f",distp,distp2);
			}
//#endif
			
			if(distp<=eps)
				continue;

			prob_t distq = q.GetNearestRadiusSQ(p[i],true,false);
//#ifdef _DEBUG
			prob_t distq2 = q.GetNearestRadiusSQ(p[i],true,true);
			if(distq != distq2)
			{
				Write2Log("not eq %.6f %.6f",distq,distq2);
			}
//#endif
			
			if(distq<=eps) 
				continue;

			dpq += log2(distq / distp) / 2.0;
		}

		dpq *= ((prob_t)p.NumDims()/p.NumElems());

		dpq += log2( (prob_t)q.NumElems() / (p.NumElems()-1.0 ) );

	#ifdef DO_TIMING
		sprintf(sMsg,"iLessP=%d iLessQ=%d",iLessP,iLessQ);
		MessageBox(0,sMsg,"WClust",MB_ICONINFORMATION);
	#endif
		
		return dpq;
	}
	catch(...)
	{
		char sMsg[1024];
		sprintf(sMsg,"KLDiv caught exception!! i=%d",i);
		MessageBox(0,sMsg,"WClust",MB_ICONERROR);
		return -666.0;
	}
}

//resistor avg.
prob_t KLDivSym(KDTreeHist& p,KDTreeHist& q)
{
	prob_t dpq = KLDiv(p,q), dqp = KLDiv(q,p);
	if(!dpq && !dqp) return 0.0;
	return dpq*dqp / (dpq+dqp);
}


//returns -1 iff num bins not equal
//otherwise returns KL divergence of histogram/distributions
//this returns "divergence" of distribution q from p
//so q should be a cluster's distribution and p should be full distribution
//make sure when passing in...
//the order matters because it is not symmetric
//however...
//In Bayesian statistics the KL divergence can be used as a measure of the information
//gain in moving from a prior distribution to a posterior distribution. If some new fact
//Y=y is discovered, it can be used to update the probability distribution for X from p(x|I)
//to a new posterior probability distribution p(x|y) using Bayes' theorem
// DKL(p(x|y)||p(x|I)) = sum( p(x|y) * log( p(x|y) / p(x|I) )
//so p can be cluster distribution and q can be full distribution
prob_t KLDiv(Hist& p,Hist& q)
{
	if(p.NumBins() != q.NumBins())
	{
		return -1.0;
	}

	prob_t d = 0.0;

	int i = 0 , iBins = p.NumBins();

	prob_t eps = 1e-100;

	for(i=0;i<iBins;i++)
	{
		prob_t pp = p.BinProb(i); //probability of bin i in distribution p
		
		if(pp<=eps) 
			continue;
		
		prob_t qp = q.BinProb(i); //probability of bin i in distribution q
		
		if(qp<=eps) 
			continue;
		
		d += pp * ( log2(pp) - log2(qp) );
	}

/*#ifdef _DEBUG
	if(d<0.0)
	{
		prob_t pf = FullProb(p);
		prob_t qf = FullProb(q);
		FILE* fp = fopen("kl_prob.txt","w");
		fprintf(fp,"p:\n");
		p.Print(fp);
		fprintf(fp,"q:\n");
		q.Print(fp);
		fclose(fp);
		int moo=0;
	}
#endif*/

	return d;
}

prob_t ResistorAvg(prob_t& p,prob_t& q)
{	if(!p && !q) return 0.0f;
	return p*q/(p+q);
}

//symmetrized form of KL-divergence
prob_t ResistorAvg(Hist& p,Hist& q)
{
	prob_t d12 = KLDiv(p,q), d21 = KLDiv(q,p);
	return (d12 * d21) / (d12 + d21);
}

//vZeroes = vector of dimensions that shouldn't contribute to calculations
//vZeroes[i] == 1 , iff you want to exclude dimension i
void CalcUDDist(vector< vector<Hist> >& vDistribs,int iClusts,vector<prob_t>& vcInf,vector< vector<prob_t> >& vcInfInter,vector<int>& vCounts,int iElems,vector<int>& vZeroes,bool bUseCounts)
{
	if(vDistribs.size() != iClusts + 2) return;

	vcInf = vector<prob_t>(iClusts+1);

	vcInfInter = vector< vector<prob_t> >(iClusts+1);
	int iC=1;
	for(iC=1;iC<=iClusts;iC++) vcInfInter[iC] = vector<prob_t>(iClusts+1);

	int iDims = vDistribs[1].size() , iD = 0;

	//uniqueness from full distribution for each cluster
	int iC1=1,iC2=1;
	for(iC1=1;iC1<=iClusts;iC1++)
	{
		prob_t kldiv=0.0;
		for(iD=0;iD<iDims;iD++)
		{
			if(vZeroes[iD]) continue;

			kldiv += KLDiv(vDistribs[iC1][iD],vDistribs[iClusts+1][iD]);
		}
		vcInf[iC1] = kldiv;
	}

	//inter-cluster distinction measures, KL divergence between
	//a cluster and cluster+other_cluster
	for(iC1=1;iC1<=iClusts;iC1++)
	{
		for(iC2=1;iC2<=iClusts;iC2++)
		{
			if(iC1==iC2)
				vcInfInter[iC1][iC2]=0.0;
			else
			{
				for(iD=0;iD<iDims;iD++)
				{
					if(vZeroes[iD]) continue;

					Hist oTmp = vDistribs[iC1][iD];
					oTmp.Add(vDistribs[iC2][iD]);
					vcInfInter[iC1][iC2] += KLDiv(vDistribs[iC1][iD],oTmp);
				}
			}				
		}
	}
	//add smallest inter-cluster KL-div
	for(iC1=1;iC1<=iClusts;iC1++)
	{
		prob_t dMinInter = 9e10;
		bool bFound = false;
		if(vCounts[iC1])
		for(iC2=1;iC2<=iClusts;iC2++)
		{
			if(iC1 != iC2 && vCounts[iC2] && vcInfInter[iC1][iC2] < dMinInter)
			{
				dMinInter = vcInfInter[iC1][iC2];
				bFound = true;
			}
		}
		if(bFound)
			vcInf[iC1] += dMinInter;
	}

	if(bUseCounts)
	{
		for(iC1=1;iC1<=iClusts;iC1++)
		{
			vcInf[iC1] = ( (prob_t)vCounts[iC1] / iElems) * vcInf[iC1];
		}
	}
}

void CalcUDDist(vector< KDTreeHist >& vDistribs,vector< KDTreeHist >& vCompDistribs,int iClusts,vector<prob_t>& vcInf,vector< vector<prob_t> >& vcInfInter,vector<int>& vCounts)
{
	if(vDistribs.size() != iClusts + 2 || vDistribs.size()!=vCompDistribs.size()) return;

	vcInf = vector<prob_t>(iClusts+1);

	vcInfInter = vector< vector<prob_t> >(iClusts+1);
	int iC=1;
	for(iC=1;iC<=iClusts;iC++) vcInfInter[iC] = vector<prob_t>(iClusts+1);

	//uniqueness from full distribution for each cluster
	int iC1=1,iC2=1;
	for(iC1=1;iC1<=iClusts;iC1++)
		vcInf[iC1] = KLDiv(vDistribs[iC1],vCompDistribs[iC1]);//KLDivSym(vDistribs[iC1],vCompDistribs[iC1]);

	//inter-cluster distinction measures, KL divergence between
	//a cluster and cluster+other_cluster
	for(iC1=1;iC1<=iClusts;iC1++)
	{
		//for(iC2=iC1+1;iC2<=iClusts;iC2++)
		for(iC1=1;iC2<=iClusts;iC2++)
		{
			if(iC1==iC2)
				vcInfInter[iC1][iC2]=0.0;
			else
				//vcInfInter[iC1][iC2] =  vcInfInter[iC2][iC1] = KLDivSym(vDistribs[iC1],vDistribs[iC2]);
				vcInfInter[iC1][iC2] =  KLDiv(vDistribs[iC1],vDistribs[iC2]);
		}
	}
	//add smallest inter-cluster KL-div
	for(iC1=1;iC1<=iClusts;iC1++)
	{
		prob_t dMinInter = 9e10;
		bool bFound = false;
		if(vCounts[iC1])
		for(iC2=1;iC2<=iClusts;iC2++)
		{
			if(iC1 != iC2 && vCounts[iC2] && vcInfInter[iC1][iC2] < dMinInter)
			{
				dMinInter = vcInfInter[iC1][iC2];
				bFound = true;
			}
		}
		if(bFound)
			vcInf[iC1] += dMinInter;
	}
}

//calculates information gain as resistor average of all clusters against full distribution
//plus smallest resistor average between cluster and all other clusters
//vDistribs must have size of iClusts + 1
void CalcRDist(vector< vector<Hist> >& vDistribs,int iClusts,vector<prob_t>& vcInf,vector< vector<prob_t> >& vcInfInter)
{
	if(vDistribs.size() != iClusts + 2) return;

	vcInf = vector<prob_t>(iClusts+1);

	vcInfInter = vector< vector<prob_t> >(iClusts+1);
	int iC=1;
	for(iC=0;iC<=iClusts;iC++) vcInfInter[iC] = vector<prob_t>(iClusts+1);

	int iDims = vDistribs[1].size() , iD = 0;

	//only need this for first time, since its recalculated for distributions who's
	//points change, when they change
	int iC1=1,iC2=1;
	for(iC1=1;iC1<=iClusts;iC1++)
	{
		prob_t kldiv=0.0;
		for(iD=0;iD<iDims;iD++)
		{
			kldiv += ResistorAvg(vDistribs[iC1][iD],vDistribs[iClusts+1][iD]);
		}
		vcInf[iC1]=kldiv;
	}

	//fill inter-cluster resistor averages
	for(iC1=1;iC1<=iClusts;iC1++)
	{
		for(iC2=1;iC2<=iC1;iC2++)
		{
			if(iC1==iC2)
				vcInfInter[iC1][iC2]=0.0;
			else
			{
				for(iD=0;iD<iDims;iD++)
				{
					vcInfInter[iC1][iC2]+=ResistorAvg(vDistribs[iC1][iD],vDistribs[iC2][iD]);
				}
				vcInfInter[iC2][iC1]=vcInfInter[iC1][iC2];
			}				
		}
	}
	//add largest inter-cluster resistor average
	for(iC1=1;iC1<=iClusts;iC1++)
	{
		prob_t dMinInter = 9e10;
		bool bFound = false;
		for(iC2=1;iC2<=iClusts;iC2++)
		{
			if(iC1 != iC2 && vcInfInter[iC1][iC2] < dMinInter)
			{
				dMinInter = vcInfInter[iC1][iC2];
				bFound = true;
			}
		}
		if(bFound)
			vcInf[iC1] += dMinInter;
	}
}

prob_t Entropy(KDTreeHist& oTree,vector<vector< Neighbor > >& vNeighbors,int iClustID,vector<int>& vClustIDs)
{
	//ScopedTimer S("Entropy...");

	int iFast = 0,iSlow = 0;

	prob_t dEntrop = 0.0;
	int isz = vClustIDs.size() , iV = 0, iT = 0, iNN = 0;

	for(iV=0;iV<isz;iV++)
	{
		if(vClustIDs[iV]!=iClustID)
			continue;

		prob_t dDist = 0.0;
		
		vector<Neighbor>& vnn = vNeighbors[iV];
		int nsz = vnn.size();
		for(iNN=0;iNN<nsz;iNN++)
		{	Neighbor& oN = vnn[iNN];
			if(vClustIDs[oN.m_id]==iClustID && oN.m_dist>0.0)
			{
				dDist = oN.m_dist;
				iFast++;
				break;
			}
		}

		if(dDist <= 0.0)
		{
			dDist = oTree.GetNearestRadiusSQ(iT,false);
#ifdef _DEBUG
			prob_t dDist2 = oTree.GetNearestRadiusSQ(iT,true);
			if(dDist!=dDist2)
			{
				Write2Log("not eq %.6f %.6f",dDist,dDist2);
			}
#endif
			iSlow++;
		}

		if(dDist >= 0.0)
		{
			prob_t dProb = oTree.RProb(MySqrt(dDist));

			if(dProb >= 0.0)
			{
				dEntrop += dProb * log2(dProb);
			}
		}
		
		iT++;
	}
	return -dEntrop;
}

void CalcEXInfo(vector< vector< Hist> >& vDistribs,vector<prob_t>& vcInf,int iClusts,int iDim,vector<int>& vCounts,int iElems)
{
	const bool bUseCounts = false;
	int iC;
	if(!vcInf.size()) vcInf = vector<prob_t>(iClusts+1);
	for(iC=1;iC<=iClusts;iC++)
	{
		prob_t kldiv=0.0;
		int iD=0;
		for(iD=0;iD<iDim;iD++)
		{
			kldiv += KLDivApproxExclusiveProb(vDistribs[iC][iD],vDistribs[iClusts+1][iD]);
		}
		if(bUseCounts)
			vcInf[iC] = ( (prob_t)vCounts[iC] / (prob_t)iElems ) * kldiv;
		else
			vcInf[iC] = kldiv;
	}
}
