// exectubale adapted from root Macro by M. Kenzie adapted from P. Meridiani.
// L. Corpe 07/2015


#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "TFile.h"
#include "TMath.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TLegend.h"

#include "TStopwatch.h"
#include "RooWorkspace.h"
#include "RooDataSet.h"
#include "RooDataHist.h"
#include "RooAddPdf.h"
#include "RooGaussian.h"
#include "RooRealVar.h"
#include "RooFormulaVar.h"
#include "RooFitResult.h"
#include "RooPlot.h"
#include "RooMsgService.h"
#include "RooMinuit.h"

#include "boost/program_options.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/predicate.hpp"

#include "TROOT.h"
#include "TStyle.h"
#include "TLatex.h"
#include "TPaveText.h"
#include "TArrow.h"

using namespace std;
using namespace RooFit;
using namespace boost;
namespace po = boost::program_options;

string filename_;
string outfilename_;
string datfilename_;
int m_hyp_;
string procString_;
int ncats_;
int binning_;
string flashggCatsStr_;
vector<string> flashggCats_;
bool isFlashgg_;
bool blind_;
int sqrts_;
bool doTable_;
bool verbose_;
bool doCrossCheck_;

void OptionParser(int argc, char *argv[]){
	po::options_description desc1("Allowed options");
	desc1.add_options()
		("help,h",                                                                                "Show help")
		("infilename,i", po::value<string>(&filename_),                                           "Input file name")
		("outfilename,o", po::value<string>(&outfilename_),                                           "Output file name")
		("mass,m", po::value<int>(&m_hyp_)->default_value(125),                                    "Mass to run at")
		("sqrts", po::value<int>(&sqrts_)->default_value(13),                                    "CoM energy")
		("binning", po::value<int>(&binning_)->default_value(70),                                    "CoM energy")
		("procs,p", po::value<string>(&procString_)->default_value("ggh,vbf,wh,zh,tth"),          "Processes")
		("isFlashgg",	po::value<bool>(&isFlashgg_)->default_value(true),													"Use flashgg format")
		("blind",	po::value<bool>(&blind_)->default_value(true),													"blind analysis")
		("doTable",	po::value<bool>(&doTable_)->default_value(false),													"doTable analysis")
		("doCrossCheck",	po::value<bool>(&doCrossCheck_)->default_value(false),													"output additional details")
		("verbose",	po::value<bool>(&verbose_)->default_value(false),													"output additional details")
		("flashggCats,f", po::value<string>(&flashggCatsStr_)->default_value("DiPhotonUntaggedCategory_0,DiPhotonUntaggedCategory_1,DiPhotonUntaggedCategory_2,DiPhotonUntaggedCategory_3,DiPhotonUntaggedCategory_4,VBFTag_0,VBFTag_1,VBFTag_2"),       "Flashgg category names to consider")
		;

	po::options_description desc2("Options kept for backward compatibility");
	desc2.add_options()
		("ncats,n", po::value<int>(&ncats_)->default_value(9),																			"Number of cats (Set Automatically if using --isFlashgg 1)")
		;
	po::options_description desc("Allowed options");
	desc.add(desc1).add(desc2);

	po::variables_map vm;
	po::store(po::parse_command_line(argc,argv,desc),vm);
	po::notify(vm);
	if (vm.count("help")){ cout << desc << endl; exit(1);}
}

map<string,RooDataSet*> getGlobeData(RooWorkspace *work, int ncats, int m_hyp){

	map<string,RooDataSet*> result;

	for (int cat=0; cat<ncats; cat++){
		result.insert(pair<string,RooDataSet*>(Form("cat%d",cat),(RooDataSet*)work->data(Form("sig_mass_m%3d_cat%d",m_hyp,cat))));
	}
	result.insert(pair<string,RooDataSet*>("all",(RooDataSet*)work->data(Form("sig_mass_m%3d_AllCats",m_hyp))));

	return result;
}

map<string,RooDataSet*> getFlashggData(RooWorkspace *work, int ncats, int m_hyp){

	map<string,RooDataSet*> result;

	for (int cat=0; cat<ncats; cat++){
	std::cout << "[DEBUG] getting dataset : " << Form("sig_mass_m%3d_%s",m_hyp,flashggCats_[cat].c_str()) << std::endl; 
		result.insert(pair<string,RooDataSet*>(Form("%s",flashggCats_[cat].c_str()),(RooDataSet*)work->data(Form("sig_mass_m%3d_%s",m_hyp,flashggCats_[cat].c_str()))));
	}
	result.insert(pair<string,RooDataSet*>("all",(RooDataSet*)work->data(Form("sig_mass_m%3d_AllCats",m_hyp))));

	return result;
}
map<string,RooAddPdf*> getGlobePdfs(RooWorkspace *work, int ncats){

	map<string,RooAddPdf*> result;
	for (int cat=0; cat<ncats; cat++){
		result.insert(pair<string,RooAddPdf*>(Form("cat%d",cat),(RooAddPdf*)work->pdf(Form("sigpdfrelcat%d_allProcs",cat))));
	}
	result.insert(pair<string,RooAddPdf*>("all",(RooAddPdf*)work->pdf("sigpdfrelAllCats_allProcs")));

	return result;
}

map<string,RooAddPdf*> getFlashggPdfs(RooWorkspace *work, int ncats){

	map<string,RooAddPdf*> result;
	for (int cat=0; cat<ncats; cat++){
	std::cout << "[DEBUG] getting dataset : " << (RooAddPdf*)work->pdf(Form("sigpdfrel%s_    allProcs",flashggCats_[cat].c_str()))<< std::endl; 
		result.insert(pair<string,RooAddPdf*>(Form("%s",flashggCats_[cat].c_str()),(RooAddPdf*)work->pdf(Form("sigpdfrel%s_allProcs",flashggCats_[cat].c_str()))));
	}
	result.insert(pair<string,RooAddPdf*>("all",(RooAddPdf*)work->pdf("sigpdfrelAllCats_allProcs")));

	return result;
}

void printInfo(map<string,RooDataSet*> data, map<string,RooAddPdf*> pdfs){

	for (map<string,RooDataSet*>::iterator dat=data.begin(); dat!=data.end(); dat++){
		if (!dat->second) {
			cout << "dataset for " << dat->first << " not found" << endl;
			exit(1);
		}
		cout << dat->first << " : ";
		dat->second->Print();
	}
	for (map<string,RooAddPdf*>::iterator pdf=pdfs.begin(); pdf!=pdfs.end(); pdf++){
		if (!pdf->second) {
			cout << "pdf for " << pdf->first << " not found" << endl;
			exit(1);
		}
		cout << pdf->first << " : ";
		pdf->second->Print();
	}

}

pair<double,double> getEffSigma(RooRealVar *mass, RooAbsPdf *pdf, double wmin=110., double wmax=130., double step=0.002, double epsilon=1.e-4){

	RooAbsReal *cdf = pdf->createCdf(RooArgList(*mass));
	cout << "Computing effSigma...." << endl;
	TStopwatch sw;
	sw.Start();
	double point=wmin;
	vector<pair<double,double> > points;

	while (point <= wmax){
		mass->setVal(point);
		if (pdf->getVal() > epsilon){
			points.push_back(pair<double,double>(point,cdf->getVal())); 
		}
		point+=step;
	}
	double low = wmin;
	double high = wmax;
	double width = wmax-wmin;
	for (unsigned int i=0; i<points.size(); i++){
		for (unsigned int j=i; j<points.size(); j++){
			double wy = points[j].second - points[i].second;
			if (TMath::Abs(wy-0.683) < epsilon){
				double wx = points[j].first - points[i].first;
				if (wx < width){
					low = points[i].first;
					high = points[j].first;
					width=wx;
				}
			}
		}
	}
	sw.Stop();
	cout << "effSigma: [" << low << "-" << high << "] = " << width/2. << endl;
	cout << "\tTook: "; sw.Print();
	pair<double,double> result(low,high);
	return result;
}

// get effective sigma from finely binned histogram
pair<double,double> getEffSigBinned(RooRealVar *mass, RooAbsPdf *pdf, double wmin=110., double wmax=130.,int stepsize=1 ){

	int nbins = int((wmax-wmin)/0.001/double(stepsize));
	TH1F *h = new TH1F("h","h",nbins,wmin,wmax);
	pdf->fillHistogram(h,RooArgList(*mass));

	double narrowest=1000.;
	double bestInt;
	int lowbin;
	int highbin;

	double oneSigma=1.-TMath::Prob(1,1);

	TStopwatch sw;
	sw.Start();
	// get first guess
	cout << "Getting first guess info. stepsize (MeV) = " << stepsize*100 << endl;
	for (int i=0; i<h->GetNbinsX(); i+=(stepsize*100)){
		for (int j=i; j<h->GetNbinsX(); j+=(stepsize*100)){
			double integral = h->Integral(i,j)/h->Integral();
			if (integral<oneSigma) continue;
			double width = h->GetBinCenter(j)-h->GetBinCenter(i);
			if (width<narrowest){
				narrowest=width;
				bestInt=integral;
				lowbin=i;
				highbin=j;
				i++;
			}
		}
	}
	cout << "Took: "; sw.Print(); 
	// narrow down result
	int thisStepSize=32;
	cout << "Narrowing....." << endl;
	while (thisStepSize>stepsize) {
		cout << "\tstepsize (MeV) = " << thisStepSize << endl;
		for (int i=(lowbin-10*thisStepSize); i<(lowbin+10*thisStepSize); i+=thisStepSize){
			for (int j=(highbin-10*thisStepSize); j<(highbin+10*thisStepSize); j+=thisStepSize){
				double integral = h->Integral(i,j)/h->Integral();
				if (integral<oneSigma) continue;
				double width = h->GetBinCenter(j)-h->GetBinCenter(i);
				if (width<narrowest){
					narrowest=width;
					bestInt=integral;
					lowbin=i;
					highbin=j;
					i++;
				}
			}
		}
		thisStepSize/=2;
	}

	sw.Stop();
	cout << narrowest/2. << " " << bestInt << " [" << h->GetBinCenter(lowbin) << "," << h->GetBinCenter(highbin) << "]" << endl;
	cout << "Took:"; sw.Print();
	pair<double,double> result(h->GetBinCenter(lowbin),h->GetBinCenter(highbin));
	delete h;
	return result;
}

// get FWHHM
vector<double> getFWHM(RooRealVar *mass, RooAbsPdf *pdf, RooDataSet *data, double wmin=110., double wmax=130., double step=0.0004) {

	cout << "Computing FWHM...." << endl;
	double nbins = (wmax-wmin)/step;
	TH1F *h = new TH1F("h","h",int(floor(nbins+0.5)),wmin,wmax);
	if (data){
		pdf->fillHistogram(h,RooArgList(*mass),data->sumEntries());
	}
	else {
		pdf->fillHistogram(h,RooArgList(*mass));
	}

	double hm = h->GetMaximum()*0.5;
	double low = h->GetBinCenter(h->FindFirstBinAbove(hm));
	double high = h->GetBinCenter(h->FindLastBinAbove(hm));

	cout << "FWHM: [" << low << "-" << high << "] Max = " << hm << endl;
	vector<double> result;
	result.push_back(low);
	result.push_back(high);
	result.push_back(hm);
	result.push_back(h->GetBinWidth(1));

	delete h;
	return result;
}

void performClosure(RooRealVar *mass, RooAbsPdf *pdf, RooDataSet *data, string closurename, double wmin=110., double wmax=130., double slow=110., double shigh=130., double step=0.002) {

	// plot to perform closure test
	cout << "Performing closure test... for " << closurename << endl; 
	double nbins = (wmax-wmin)/step;
	TH1F *h = new TH1F("h","h",int(floor(nbins+0.5)),wmin,wmax);
	if (data){
		pdf->fillHistogram(h,RooArgList(*mass),data->sumEntries());
		h->Scale(2*h->GetNbinsX()/double(binning_));
	}
	else {
		pdf->fillHistogram(h,RooArgList(*mass));
	}
	int binLow = h->FindBin(slow);
	int binHigh = h->FindBin(shigh)-1;
	TH1F *copy = new TH1F("copy","c",binHigh-binLow,h->GetBinLowEdge(binLow),h->GetBinLowEdge(binHigh+1));
	for (int b=0; b<copy->GetNbinsX(); b++) copy->SetBinContent(b+1,h->GetBinContent(b+1+binLow));
	double areaCov = 100*h->Integral(binLow,binHigh)/h->Integral();

	// style
	h->SetLineColor(kBlue);
	h->SetLineWidth(3);
	h->SetLineStyle(7);
	copy->SetLineWidth(3);
	copy->SetFillColor(kGray);

	RooPlot *plot;
	TCanvas *c = new TCanvas();
	if (data){
		plot = (mass->frame(Bins(binning_),Range("higgsRange")));
		plot->addTH1(h,"hist"); 
		plot->addTH1(copy,"same f");
		if (data) data->plotOn(plot);
		pdf->plotOn(plot,Normalization(h->Integral(),RooAbsReal::NumEvent),NormRange("higgsRange"),Range("higgsRange"),LineWidth(1),LineColor(kRed),LineStyle(kDashed));
		plot->Draw();
		c->Print(closurename.c_str());
	}
	else {
		plot = mass->frame(Bins(binning_),Range("higgsRange"));
		h->Scale(plot->getFitRangeBinW()/h->GetBinWidth(1));
		copy->Scale(plot->getFitRangeBinW()/h->GetBinWidth(1));
		pdf->plotOn(plot,LineColor(kRed),LineWidth(3));
		plot->Draw();
		h->Draw("hist same");
		copy->Draw("same f");
		c->Print(closurename.c_str());
	}
	cout << "IntH: [" << h->GetBinLowEdge(binLow) << "-" << h->GetBinLowEdge(binHigh+1) << "] Area = " << areaCov << endl;
	delete c;
	//  delete copy;
	// delete h;
	delete plot;
}

void Plot(RooRealVar *mass, RooDataSet *data, RooAbsPdf *pdf, pair<double,double> sigRange, vector<double> fwhmRange, string title, string savename){

	double semin=sigRange.first;
	double semax=sigRange.second;
	double fwmin=fwhmRange[0];
	double fwmax=fwhmRange[1];
	double halfmax=fwhmRange[2];
	double binwidth=fwhmRange[3];

	RooPlot *plot = mass->frame(Bins(binning_),Range("higgsRange"));
	if (data) data->plotOn(plot,Invisible());
	pdf->plotOn(plot,NormRange("higgsRange"),Range(semin,semax),FillColor(19),DrawOption("F"),LineWidth(2),FillStyle(1001),VLines(),LineColor(15));
	TObject *seffLeg = plot->getObject(int(plot->numItems()-1));
	pdf->plotOn(plot,NormRange("higgsRange"),Range(semin,semax),LineColor(15),LineWidth(2),FillStyle(1001),VLines());
	pdf->plotOn(plot,NormRange("higgsRange"),Range("higgsRange"),LineColor(kBlue),LineWidth(2),FillStyle(0));
	TObject *pdfLeg = plot->getObject(int(plot->numItems()-1));
	if (data) data->plotOn(plot,MarkerStyle(kOpenSquare));
	TObject *dataLeg = plot->getObject(int(plot->numItems()-1));
	TLegend *leg = new TLegend(0.15,0.89,0.5,0.55);
	leg->SetFillStyle(0);
	leg->SetLineColor(0);
	leg->SetTextSize(0.03);
	if (data) leg->AddEntry(dataLeg,"Simulation","lep");
	leg->AddEntry(pdfLeg,"Parametric model","l");
	leg->AddEntry(seffLeg,Form("#sigma_{eff} = %1.2f GeV",0.5*(semax-semin)),"fl");

	plot->GetXaxis()->SetNdivisions(509);
	halfmax*=(plot->getFitRangeBinW()/binwidth);
	TArrow *fwhmArrow = new TArrow(fwmin,halfmax,fwmax,halfmax,0.02,"<>");
	fwhmArrow->SetLineWidth(2.);
	TPaveText *fwhmText = new TPaveText(0.15,0.45,0.45,0.58,"brNDC");
	fwhmText->SetFillColor(0);
	fwhmText->SetLineColor(kWhite);
	fwhmText->SetTextSize(0.03);
	fwhmText->AddText(Form("FWHM = %1.2f GeV",(fwmax-fwmin)));
  std::cout << " [FOR TABLE] Tag " << data->GetName() << "=, Mass " << mass->getVal() << " sigmaEff=" << 0.5*(semax-semin) << "= , FWMH=" << (fwmax-fwmin)/2.35 << "=" << std::endl;

	TLatex lat1(0.65,0.85,"#splitline{CMS Preliminary}{Simulation}");
	lat1.SetNDC(1);
	lat1.SetTextSize(0.03);
	TLatex lat2(0.65,0.75,title.c_str());
	lat2.SetNDC(1);
	lat2.SetTextSize(0.025);

	TCanvas *canv = new TCanvas("c","c",600,600);
	plot->SetTitle("");
	plot->GetXaxis()->SetTitle("m_{#gamma#gamma} (GeV)");
	plot->Draw();
	leg->Draw("same");
	fwhmArrow->Draw("same <>");
	fwhmText->Draw("same");
	lat1.Draw("same");
	lat2.Draw("same");
	canv->Print(Form("%s.pdf",savename.c_str()));
	canv->Print(Form("%s.png",savename.c_str()));
	string path = savename.substr(0,savename.find('/'));
	canv->Print(Form("%s/animation.gif+100",path.c_str()));
	delete canv;

}

int main(int argc, char *argv[]){

	OptionParser(argc,argv);

	TStopwatch sw;
	sw.Start();

	RooMsgService::instance().setGlobalKillBelow(RooFit::ERROR);
	RooMsgService::instance().setSilentMode(true);

	system("mkdir -p plots/SignalPlots/");

	vector<string> procs;
	split(procs,procString_,boost::is_any_of(","));
	split(flashggCats_,flashggCatsStr_,boost::is_any_of(","));

	if (isFlashgg_){
		ncats_ =flashggCats_.size();
		// Ensure that the loop over the categories does not go out of scope. 
	 std::cout << "[INFO] consider "<< ncats_ <<" tags/categories" << std::endl;
	}


	gROOT->SetBatch();
	gStyle->SetTextFont(42);


	TFile *hggFile = TFile::Open(filename_.c_str());
	RooWorkspace *hggWS;
	hggWS = (RooWorkspace*)hggFile->Get(Form("wsig_%dTeV",sqrts_));

	if (!hggWS) {
		cerr << "Workspace is null" << endl;
		exit(1);
	}

	RooRealVar *mass= (RooRealVar*)hggWS->var("CMS_hgg_mass");

	RooRealVar *mh = (RooRealVar*)hggWS->var("MH");
	mh->setVal(m_hyp_);
	mass->setRange("higgsRange",m_hyp_-20.,m_hyp_+15.);

	map<string,RooDataSet*> dataSets;
	map<string,RooAddPdf*> pdfs;

	if (isFlashgg_){
		dataSets = getFlashggData(hggWS,ncats_,m_hyp_);
		pdfs = getFlashggPdfs(hggWS,ncats_);
	}
	else {
		dataSets = getGlobeData(hggWS,ncats_,m_hyp_);
		pdfs = getGlobePdfs(hggWS,ncats_); 
	}

//	printInfo(dataSets,pdfs);

	map<string,double> sigEffs;
	map<string,double> fwhms;


	system(Form("mkdir -p %s",outfilename_.c_str()));
	system(Form("rm -f %s/animation.gif",outfilename_.c_str()));


	for (map<string,RooDataSet*>::iterator dataIt=dataSets.begin(); dataIt!=dataSets.end(); dataIt++){
		pair<double,double> thisSigRange = getEffSigma(mass,pdfs[dataIt->first],m_hyp_-10.,m_hyp_+10.);
		//pair<double,double> thisSigRange = getEffSigBinned(mass,pdf[dataIt->first],m_hyp_-10.,m_hyp_+10);
		vector<double> thisFWHMRange = getFWHM(mass,pdfs[dataIt->first],dataIt->second,m_hyp_-10.,m_hyp_+10.);
		sigEffs.insert(pair<string,double>(dataIt->first,(thisSigRange.second-thisSigRange.first)/2.));
		fwhms.insert(pair<string,double>(dataIt->first,thisFWHMRange[1]-thisFWHMRange[0]));
		if (doCrossCheck_) performClosure(mass,pdfs[dataIt->first],dataIt->second,Form("%s/closure_%s.pdf",outfilename_.c_str(),dataIt->first.c_str()),m_hyp_-10.,m_hyp_+10.,thisSigRange.first,thisSigRange.second);
		Plot(mass,dataIt->second,pdfs[dataIt->first],thisSigRange,thisFWHMRange,dataIt->first,Form("%s/%s",outfilename_.c_str(),dataIt->first.c_str()));
	}


	map<string,pair<double,double> > bkgVals;
	map<string,vector<double> > sigVals;
	map<string,pair<double,double> > datVals;
	map<string,double> sobVals;


	hggFile->Close();
}









