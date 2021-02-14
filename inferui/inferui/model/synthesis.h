/*
   Copyright 2018 Software Reliability Lab, ETH Zurich

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef CC_SYNTHESIS_SYNTHESIS_H
#define CC_SYNTHESIS_SYNTHESIS_H


#include "inferui/model/model.h"
#include "inferui/model/constraints.h"
#include "base/iterutil.h"
#include "inferui/model/constraint_model.h"

//DECLARE_bool(uniform_probability);

//template<class Model>
class ConstraintCache {
public:
  ConstraintCache(const ProbModel* model, std::vector<View>& views, const Orientation& orientation) : model_(model), orientation_(orientation), allowed_targets(views.size(), std::vector<bool>(views.size(), false)) {
    InitializeBaseConstraints(views);

    InitializePrune();
  }

  ConstraintCache(const ProbModel* model, std::vector<View>& views, std::vector<App>& apps, const Orientation& orientation)
      : model_(model), orientation_(orientation), allowed_targets(views.size(), std::vector<bool>(views.size(), false)) {
//    candidate_attrs_.emplace_back(InitializeBaseConstraints());
    if (apps.empty()) {
      InitializeBaseConstraints(views);
    } else {
      InitializeBaseConstraints(views, apps);
    }

    InitializePrune();
  }

  void InitializePrune() {
    for (auto it = begin(); it != end(); it++) {
      const Attribute& attr = *it;

      if (IsRelationalAnchor(attr.type)) {
        if (!allowed_targets[attr.tgt_primary->pos][attr.src->pos]) {
          allowed_targets[attr.src->pos][attr.tgt_primary->pos] = true;
        }
      } else {
        if (!allowed_targets[attr.tgt_primary->pos][attr.src->pos] && !allowed_targets[attr.tgt_secondary->pos][attr.src->pos]) {
          allowed_targets[attr.src->pos][attr.tgt_primary->pos] = true;
          allowed_targets[attr.src->pos][attr.tgt_secondary->pos] = true;
        }
      }
    }

  }

  bool IsValid(const Attribute* attr) const {
    ConstraintType type = attr->type;
    if (IsRelationalAnchor(type)) {
      return !(attr->tgt_primary->pos == 0 && (type == ConstraintType::T2B || type == ConstraintType::B2T ||
                               type == ConstraintType::L2R || type == ConstraintType::R2L));
    } else {
      if ((type == ConstraintType::L2RxR2L || type == ConstraintType::T2BxB2T) && *attr->tgt_primary == *attr->tgt_secondary) {
        return false;
      }

      if (attr->tgt_primary->pos == 0 && attr->tgt_secondary->pos == 0 && (type == ConstraintType::L2LxR2L || type == ConstraintType::L2RxR2R ||
                                         type == ConstraintType::T2TxB2T || type == ConstraintType::T2BxB2B)) {
        return false;
      }

      return true;
    }
  }

  bool IsAllowed(const Attribute* attr) const {
    if (!IsValid(attr)) return false;
    const Orientation& orientation = ConstraintTypeToOrientation(attr->type);
    if (Contains(attr->src->view_size, orientation) && attr->src->view_size.find(orientation)->second != attr->view_size) {
      return false;
    }
    if (IsRelationalAnchor(attr->type)) {
      return allowed_targets[attr->src->pos][attr->tgt_primary->pos];
    } else {
      return allowed_targets[attr->src->pos][attr->tgt_primary->pos] && allowed_targets[attr->src->pos][attr->tgt_secondary->pos];
    }

  }

  using iterator = MultiSortedIterator<Attribute>;

  iterator begin() const {
    return MultiSortedIterator<Attribute>(candidate_attrs_);
  }

  iterator end() const {
    return MultiSortedIterator<Attribute>(candidate_attrs_, true);
  }

  size_t size() const {
    size_t size = 0;
    for (const auto& candidates : candidate_attrs_) {
      size += candidates.size();
    }
    return size;
  }

  void Dump() const {
    int i = 0;
    for (const auto& attrs : candidate_attrs_) {
      LOG(INFO) << "Candidates: " << i++;
      for (const auto& attr : attrs) {
        LOG(INFO) << '\t' << attr;
      }
    }
  }

  int NumConstraints(int view_id) const {
    CHECK_GE(view_id - 1, 0);
    CHECK_LT(view_id - 1, candidate_attrs_.size());
    const std::vector<Attribute>& attrs = candidate_attrs_.at(view_id - 1); //there no entry for content frame
    return attrs.size();
  }

  void DumpTopN(int view_id, int count, std::vector<View>& views) const {
    CHECK_GE(view_id - 1, 0);
    CHECK_LT(view_id - 1, candidate_attrs_.size());
    const std::vector<Attribute>& attrs = candidate_attrs_.at(view_id - 1); //there no entry for content frame
    for (int i = 0; i < count && i < static_cast<int>(attrs.size()); i++) {
      LOG(INFO) << '\t' << attrs[i];
//      LOG(INFO) << "\t" << model_.DebugProb(attrs[i], views);
    }
  }

  Attribute BestAttribute(int view_id) const {
    CHECK_GE(view_id - 1, 0);
    CHECK_LT(view_id - 1, candidate_attrs_.size());
    const std::vector<Attribute>& attrs = candidate_attrs_.at(view_id - 1); //there no entry for content frame
    return attrs[0];
  }

  std::pair<int, double> GetRank(int id, const Attribute& attr) const {
    CHECK_GE(id - 1, 0);
    CHECK_LT(id - 1, candidate_attrs_.size());
//    for (size_t id = 0; id < candidate_attrs_.size(); id ++) {
//      if (candidate_attrs_[id].at(0).src->id == attr.src->id) {
//
//      }
//    }
//    LOG(INFO) << id << " " << attr;
    CHECK_GT(candidate_attrs_[id - 1].size(), 0);
    CHECK(candidate_attrs_[id - 1].at(0).src->id == attr.src->id);
    return GetRank(id, attr.type, attr.view_size, attr.tgt_primary->id, (attr.tgt_secondary == nullptr) ? -1 : attr.tgt_secondary->id);
  }

  const Attribute* GetAttr(int id, int rank) const {
    CHECK_GE(id - 1, 0);
    CHECK_LT(id - 1, candidate_attrs_.size());
    const std::vector<Attribute>& attrs = candidate_attrs_.at(id - 1); //there no entry for content frame
    if (rank >= static_cast<int>(attrs.size())) return nullptr;
    return &attrs[rank];
  }

  std::pair<int, double> GetRank(int id, ConstraintType type, ViewSize view_size, int primary_tgt, int secondary_tgt, int max_rank = -1) const {
    CHECK_GE(id - 1, 0);
    CHECK_LT(id - 1, candidate_attrs_.size());
    const std::vector<Attribute>& attrs = candidate_attrs_.at(id - 1); //there no entry for content frame
    for (size_t rank = 0; rank < attrs.size(); rank++) {
      if (max_rank != -1 && static_cast<int>(rank) > max_rank) return std::make_pair(std::numeric_limits<int>::max(), -9999);
      const Attribute& attr = attrs[rank];
      if (attr.type == type && attr.view_size == view_size && attr.tgt_primary->id == primary_tgt &&
          (attr.tgt_secondary == nullptr || attr.tgt_secondary->id == secondary_tgt)) {
//        LOG(INFO) << "Found: " << attr;
        return std::make_pair(rank, attr.prob); //rank
      }
    }
    return std::make_pair(std::numeric_limits<int>::max(), -9999);
//    LOG(FATAL) << "Attribute not found";
  }

private:

  std::vector<Attribute> GenAttributesForApp(View& view, std::vector<View>& views) {
    ConstraintGenerator gen;
    std::vector<Attribute> attrs;
    gen.GenFixedSizeRelationalConstraints(orientation_, view, views, [&attrs](Attribute&& attr) {
      attrs.emplace_back(attr);
    });

    gen.GenFixedSizeCenteringConstraints(orientation_, view, views, [&attrs](Attribute&& attr) {
      attrs.emplace_back(attr);
    });

    gen.GenMatchConstraintCenteringConstraints(orientation_, view, views, [&attrs](Attribute&& attr) {
      attrs.emplace_back(attr);
    });
    return attrs;
  }


  void InitializeBaseConstraints(std::vector<View>& views) {
    candidate_attrs_.clear();
    ConstraintGenerator gen;
    for (auto& view : views) {
      if (view.is_content_frame()) continue;
      std::vector<Attribute> attrs;
      gen.GenFixedSizeRelationalConstraints(orientation_, view, views, [&attrs](Attribute&& attr) {
        attrs.emplace_back(attr);
      });

      gen.GenFixedSizeCenteringConstraints(orientation_, view, views, [&attrs](Attribute&& attr) {
        attrs.emplace_back(attr);
      });

      gen.GenMatchConstraintCenteringConstraints(orientation_, view, views, [&attrs](Attribute&& attr) {
        attrs.emplace_back(attr);
      });

      for (Attribute& attr : attrs) {
        attr.prob = model_->AttrProb(attr, views);
//        LOG(INFO) << attr;
      }

      std::sort(attrs.begin(), attrs.end(), [this](const Attribute &a, const Attribute &b) {
        return a.prob > b.prob;
      });

      CHECK(!attrs.empty());
      candidate_attrs_.emplace_back(attrs);
    }
  }


  void InitializeBaseConstraints(std::vector<View>& views, std::vector<App>& apps) {
    candidate_attrs_.clear();
    CHECK(!apps.empty());

    for (auto& view : views) {
      if (view.is_content_frame()) continue;

      std::vector<Attribute> attrs = GenAttributesForApp(view, views);

      LOG(INFO) << "Base attrs: " << view;
      for (const Attribute& attr : attrs) {
        LOG(INFO) << '\t' << attr;
      }

      std::vector<bool> matches(attrs.size(), true);
      for (auto& app : apps) {
        const std::vector<Attribute>& constraint_attrs = GenAttributesForApp(app.findView(view.id), app.GetViews());
        LOG(INFO) << "Constraint attrs: " << app.findView(view.id);
        for (const Attribute& attr : constraint_attrs) {
          LOG(INFO) << '\t' << attr;
        }
        for (size_t i = 0; i < attrs.size(); i++) {
          if (!matches[i]) continue;

          if (!Contains(constraint_attrs, attrs[i])) {
            matches[i] = false;
          }
        }

        std::vector<Attribute> tmp_attrs = attrs;
        attrs.clear();
        for (size_t i = 0; i < tmp_attrs.size(); i++) {
          if (!matches[i]) continue;
          attrs.push_back(tmp_attrs[i]);
        }

        LOG(INFO) << "Final attrs: " << app.findView(view.id);
        for (const Attribute& attr : attrs) {
          LOG(INFO) << '\t' << attr;
        }
      }

      if (attrs.empty()) {
        LOG(INFO) << "Attributes empty for view: " << view;
        LOG(INFO) << views;

        for (const App& app : apps) {
          LOG(INFO) << app.GetViews();
        }
      }
      CHECK(!attrs.empty());

      for (Attribute& attr : attrs) {
        attr.prob = model_->AttrProb(attr, views);
      }
      std::sort(attrs.begin(), attrs.end(), [this](const Attribute &a, const Attribute &b) {
        return a.prob > b.prob;
      });

      candidate_attrs_.emplace_back(attrs);
    }

  }

  const ProbModel* model_;
//  std::vector<View>& views_;
  const Orientation orientation_;

  std::vector<std::vector<Attribute>> candidate_attrs_;

  std::vector<std::vector<bool>> allowed_targets;
};

//template<class Model>
class LayoutSynthesis {
public:

  LayoutSynthesis(const ProbModel* model) : model_(model) {

  }

  void SynthesizeLayout(App& app) const {
    std::vector<App> apps;
    SynthesizeLayout(app, apps);
  }

  void SynthesizeLayout(App& app, std::vector<App>& apps) const {
    if (app.GetViews().size() == 1) {
      // nothing to synthesize
      return;
    }
    //sanity check
    for (const App& other : apps) {
      CHECK_EQ(app.GetViews().size(), other.GetViews().size());
    }

    Synthesize(app.GetViews(), apps, Orientation::HORIZONTAL);
    Synthesize(app.GetViews(), apps, Orientation::VERTICAL);
  }

private:

  bool ShouldApplyConstraint(View& view, const Orientation& orientation, const Attribute& attr) const {
    if (view.IsCircularRelation(orientation, attr)) {
      VLOG(2) << "\tIsCircularRelation";
      return false;
    }

    if (!view.HasAttribute(orientation)) {
      VLOG(2) << "\tEmptyAttribute";
      return true;
    }

    bool will_anchor = view.IsAnchoredWithAttr(orientation, attr);
    if (view.IsAnchored(orientation) == will_anchor) {
      VLOG(2) << "\tAnchor EQ: " << attr.prob << " vs " << view.GetAttributeProb(orientation);
      return attr.prob > view.GetAttributeProb(orientation);
    }

    VLOG(2) << "\tWill Anchor: " << will_anchor;
    return will_anchor;
  }

  void Synthesize(std::vector<View>& views, std::vector<App>& apps, const Orientation& orientation) const {
    VLOG(2) << "Synthesizing Constraints: " << OrientationStr(orientation);
    VLOG(2) << "\t" << "Initializing Candidate Attributes for " << views.size() << " views";
    Timer timer;
    timer.Start();
    ConstraintCache cache(model_, views, apps, orientation);
    VLOG(2) << "\t\t" << "finished in " << timer.GetMilliSeconds() << "ms ";

    VLOG(2) << "Top attributes";
    int i = 0;
    for (auto it = cache.begin(); it != cache.end(); it++) {
      const Attribute& attr = *it;
      VLOG(2) << attr << "\n" << model_->DebugProb(attr, views);
      if (i++ > 10) break;
    }
    VLOG(2) << "total attributes: " << cache.size();
    VLOG(2) << "===";

    // Main Synthesis Loop
    for (auto it = cache.begin(); it != cache.end(); it++) {
      const Attribute& attr = *it;

      VLOG(2) << attr;
      if (!ShouldApplyConstraint(*attr.src, orientation, attr)) {
        continue;
      }
      VLOG(2) << "\tapply";
      attr.src->ApplyAttribute(orientation, attr);
//      it.Reset();
      if (attr.src->IsAnchored(orientation)) {
        it.SetCurrentToEnd();
      }
    }
  }

  const ProbModel* model_;
};


#endif //CC_SYNTHESIS_SYNTHESIS_H
