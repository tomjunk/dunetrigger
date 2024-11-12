////////////////////////////////////////////////////////////////////////
// Class:       TriggerAnaTree
// Plugin Type: analyzer (Unknown Unknown)
// File:        TriggerAnaTree_module.cc
//
// Generated at Fri Aug 30 14:50:19 2024 by jierans using cetskelgen
// from cetlib version 3.18.02.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileDirectory.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "detdataformats/trigger/TriggerActivityData.hpp"
#include "detdataformats/trigger/TriggerCandidateData.hpp"
#include "detdataformats/trigger/TriggerPrimitive.hpp"

#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

using dunedaq::trgdataformats::TriggerActivityData;
using dunedaq::trgdataformats::TriggerCandidateData;
using dunedaq::trgdataformats::TriggerPrimitive;

class TriggerAnaTree;
class TriggerAnaTree : public art::EDAnalyzer {
public:
  explicit TriggerAnaTree(fhicl::ParameterSet const &p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  TriggerAnaTree(TriggerAnaTree const &) = delete;
  TriggerAnaTree(TriggerAnaTree &&) = delete;
  TriggerAnaTree &operator=(TriggerAnaTree const &) = delete;
  TriggerAnaTree &operator=(TriggerAnaTree &&) = delete;

  // Required functions.
  void beginJob() override;
  void analyze(art::Event const &e) override;
  // void endJob() override;

private:
  art::ServiceHandle<art::TFileService> tfs;
  std::map<std::string, TTree *> tree_map;
  // buffers for writing to ROOT Trees
  unsigned int fEventID;
  int fRun;
  int fSubRun;
  int fAssnIdx;

  std::map<std::string, dunedaq::trgdataformats::TriggerPrimitive> tp_bufs;
  std::map<std::string, dunedaq::trgdataformats::TriggerActivityData> ta_bufs;
  std::map<std::string, dunedaq::trgdataformats::TriggerCandidateData> tc_bufs;


  bool dump_tp, dump_ta, dump_tc;

  void make_tp_tree_if_needed(std::string tag, bool assn=false);
  void make_ta_tree_if_needed(std::string tag, bool assn=false);
  void make_tc_tree_if_needed(std::string tag);
};

TriggerAnaTree::TriggerAnaTree(fhicl::ParameterSet const &p)
    : EDAnalyzer{p}, dump_tp(p.get<bool>("dump_tp")),
      dump_ta(p.get<bool>("dump_ta")), dump_tc(p.get<bool>("dump_tc"))
// More initializers here.
{}

void TriggerAnaTree::beginJob() {}

void TriggerAnaTree::analyze(art::Event const &e) {
  fRun = e.run();
  fSubRun = e.subRun();
  fEventID = e.id().event();
  if (dump_tp) {
    std::vector<art::Handle<std::vector<TriggerPrimitive>>> tpHandles =
        e.getMany<std::vector<TriggerPrimitive>>();
    for (auto const &tpHandle : tpHandles) {
      std::string tag = tpHandle.provenance()->inputTag().encode();
      std::string map_tag = "tp/" + tag;
      make_tp_tree_if_needed(tag);
      for (const TriggerPrimitive &tp : *tpHandle) {
        tp_bufs[map_tag] = tp;
        tree_map[map_tag]->Fill();
      }
    }
  }

  if (dump_ta) {
    std::vector<art::Handle<std::vector<TriggerActivityData>>> taHandles =
        e.getMany<std::vector<TriggerActivityData>>();
    for (auto const &taHandle : taHandles) {
      art::FindManyP<TriggerPrimitive> assns(taHandle, e, taHandle.provenance()->moduleLabel());
      std::string tag = taHandle.provenance()->inputTag().encode();
      std::string map_tag = "ta/" + tag;
      make_ta_tree_if_needed(tag);
      for (unsigned int i = 0; i < taHandle->size(); i++) {
        const TriggerActivityData &ta = *art::Ptr<TriggerActivityData>(taHandle, i);
        if (assns.isValid()){
          art::InputTag ta_input_tag = taHandle.provenance()->inputTag();
          std::string tpInTaTag = art::InputTag(ta_input_tag.label(), ta_input_tag.instance() + "inTAs", ta_input_tag.process()).encode();
          std::string map_tpInTaTag = "tp/" + tpInTaTag;
          make_tp_tree_if_needed(tpInTaTag, true);
          fAssnIdx = i;
          std::vector<art::Ptr<TriggerPrimitive>> matched_tps = assns.at(i);
          for (art::Ptr<TriggerPrimitive> tp : matched_tps) {
            tp_bufs[map_tpInTaTag] = *tp;
            tree_map[map_tpInTaTag]->Fill();
          }
        }
        ta_bufs[map_tag] = ta;
        tree_map[map_tag]->Fill();
      }
    }
  }

  if (dump_tc) {
    std::vector<art::Handle<std::vector<TriggerCandidateData>>> tcHandles =
        e.getMany<std::vector<TriggerCandidateData>>();
    for (auto const &tcHandle : tcHandles) {
      art::FindManyP<TriggerActivityData> assns(tcHandle, e, tcHandle.provenance()->moduleLabel());
      std::string tag = tcHandle.provenance()->inputTag().encode();
      std::string map_tag = "tc/" + tag;
      make_tc_tree_if_needed(tag);
      for (unsigned int i = 0; i < tcHandle->size(); i++) {
        const TriggerCandidateData &tc = *art::Ptr<TriggerCandidateData>(tcHandle, i);
        if (assns.isValid()){
          art::InputTag tc_input_tag = tcHandle.provenance()->inputTag();
          std::string taInTcTag = art::InputTag(tc_input_tag.label(), tc_input_tag.instance() + "inTCs", tc_input_tag.process()).encode();
          std::string map_taInTcTag = "ta/" + taInTcTag;
          make_ta_tree_if_needed(taInTcTag, true);
          fAssnIdx = i;
          std::vector<art::Ptr<TriggerActivityData>> matched_tas = assns.at(i);
          for (art::Ptr<TriggerActivityData> ta : matched_tas) {
            ta_bufs[map_taInTcTag] = *ta;
            tree_map[map_taInTcTag]->Fill();
          }
        }
        tc_bufs[map_tag] = tc;
        tree_map[map_tag]->Fill();
      }
    }
  }
}

void TriggerAnaTree::make_tp_tree_if_needed(std::string tag, bool assn) {
  std::string map_tag = "tp/" + tag;
  if (!tree_map.count(map_tag)) {
    art::TFileDirectory tp_dir =
        tfs->mkdir("TriggerPrimitives", "Trigger Primitive Trees");
    std::cout << "Creating new TTree for " << tag << std::endl;

    // Replace ":" with "_" in TTree names so that they can be used in ROOT's
    // intepreter
    std::string tree_name = tag;
    std::replace(tree_name.begin(), tree_name.end(), ':', '_');
    TTree *tree = tp_dir.make<TTree>(tree_name.c_str(), tree_name.c_str());
    tree_map[map_tag] = tree;
    TriggerPrimitive &tp = tp_bufs[map_tag];
    tree->Branch("Event", &fEventID, "Event/I");
    tree->Branch("Run", &fRun, "Run/I");
    tree->Branch("SubRun", &fSubRun, "SubRun/I");
    tree->Branch("version", &tp.version);
    tree->Branch("time_start", &tp.time_start);
    tree->Branch("time_peak", &tp.time_peak);
    tree->Branch("time_over_threshold", &tp.time_over_threshold);
    tree->Branch("channel", &tp.channel);
    tree->Branch("adc_integral", &tp.adc_integral);
    tree->Branch("adc_peak", &tp.adc_peak);
    tree->Branch("detid", &tp.detid);
    tree->Branch("type", &tp.type, "type/I");
    tree->Branch("algorithm", &tp.algorithm, "algorithm/I");
    if (assn)
      tree->Branch("TAnumber", &fAssnIdx, "TAnumber/I");
  }
}

void TriggerAnaTree::make_ta_tree_if_needed(std::string tag, bool assn) {
  std::string map_tag = "ta/" + tag;
  if (!tree_map.count(map_tag)) {
    art::TFileDirectory ta_dir =
        tfs->mkdir("TriggerActivities", "Trigger Activity Trees");
    std::cout << "Creating new TTree for " << tag << std::endl;
    // Replace ":" with "_" in TTree names so that they can be used in ROOT's
    // intepreter
    std::string tree_name = tag;
    std::replace(tree_name.begin(), tree_name.end(), ':', '_');
    TTree *tree = ta_dir.make<TTree>(tree_name.c_str(), tree_name.c_str());
    tree_map[map_tag] = tree;
    TriggerActivityData &ta = ta_bufs[map_tag];
    tree->Branch("Event", &fEventID, "Event/I");
    tree->Branch("Run", &fRun, "Run/I");
    tree->Branch("SubRun", &fSubRun, "SubRun/I");
    tree->Branch("version", &ta.version);
    tree->Branch("time_start", &ta.time_start);
    tree->Branch("time_end", &ta.time_end);
    tree->Branch("time_peak", &ta.time_peak);
    tree->Branch("time_activity", &ta.time_activity);
    tree->Branch("channel_start", &ta.channel_start);
    tree->Branch("channel_end", &ta.channel_end);
    tree->Branch("channel_peak", &ta.channel_peak);
    tree->Branch("adc_integral", &ta.adc_integral);
    tree->Branch("adc_peak", &ta.adc_peak);
    tree->Branch("detid", &ta.detid);
    // HACK: assuming enums are ints here
    tree->Branch("type", &ta.type, "type/I");
    tree->Branch("algorithm", &ta.algorithm, "algorithm/I");
    if (assn)
      tree->Branch("TCnumber", &fAssnIdx, "TCnumber/I");
  }
}

void TriggerAnaTree::make_tc_tree_if_needed(std::string tag) {
  std::string map_tag = "tc/" + tag;
  if (!tree_map.count(map_tag)) {
    art::TFileDirectory tc_dir =
        tfs->mkdir("TriggerCandidates", "Trigger Candidate Trees");
    std::cout << "Creating new TTree for " << tag << std::endl;
    // Replace ":" with "_" in TTree names so that they can be used in ROOT's
    // intepreter
    std::string tree_name = tag;
    std::replace(tree_name.begin(), tree_name.end(), ':', '_');
    TTree *tree = tc_dir.make<TTree>(tree_name.c_str(), tree_name.c_str());
    tree_map[map_tag] = tree;
    TriggerCandidateData &tc = tc_bufs[map_tag];
    tree->Branch("Event", &fEventID, "Event/I");
    tree->Branch("Run", &fRun, "Run/I");
    tree->Branch("SubRun", &fSubRun, "SubRun/I");
    tree->Branch("version", &tc.version);
    tree->Branch("time_start", &tc.time_start);
    tree->Branch("time_end", &tc.time_end);
    tree->Branch("time_candidate", &tc.time_candidate);
    tree->Branch("detid", &tc.detid);
    tree->Branch("type", &tc.type, "type/I");
    tree->Branch("algorithm", &tc.algorithm, "type/I");
  }
}

DEFINE_ART_MODULE(TriggerAnaTree)
