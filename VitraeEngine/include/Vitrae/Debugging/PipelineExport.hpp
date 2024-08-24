#pragma once

#include "Vitrae/Pipelines/Pipeline.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <set>

namespace Vitrae
{

/**
 * @brief Exports a graph depicting the pipeline in the dot language
 * @param pipeline The pipeline to export
 * @param out The output stream to which to write
 */
template <TaskChild BasicTask>
void exportPipeline(const Pipeline<BasicTask> &pipeline, std::ostream &out)
{
    using Pipeline = Pipeline<BasicTask>;
    using Pipeitem = Pipeline::PipeItem;

    auto escapedLabel = [&](const String &label) -> String {
        String ret = searchAndReplace(label, "\"", "\\\"");
        ret = searchAndReplace(ret, "\n", "\\n");
        return ret;
    };
    auto getInputPropId = [&](const PropertySpec &spec) -> String {
        return String("Prop_") + spec.name;
    };
    auto getOutputPropId = [&](const PropertySpec &spec) -> String {
        return String("Prop_") + spec.name;
    };
    auto getTaskId = [&](const BasicTask &task) -> String {
        return String("Task_") + std::to_string(std::hash<StringView>{}(task.getFriendlyName()));
    };
    auto outputTaskNode = [&](StringView id, std::size_t ord, const BasicTask &task) {
        out << id << " [";
        out << "label=\""
            << escapedLabel(String("#") + std::to_string(ord) + "\n" +
                            String(task.getFriendlyName()))
            << "\", ";
        out << "shape=box, ";
        out << "style=\"filled\", ";
        out << "fillcolor=\"lightblue\"";
        out << "bgcolor=\"lightblue\"";
        out << "];\n";
    };
    auto outputPropNode = [&](StringView id, const PropertySpec &spec) {
        out << id << " [";
        out << "label=\"" << escapedLabel(spec.name) << "\", ";
        out << "shape=box, ";
        out << "style=\"rounded,filled\", ";
        out << "fillcolor=\"lightyellow\"";
        out << "bgcolor=\"lightyellow\"";
        out << "];\n";
    };
    auto outputConnection = [&](StringView from, StringView to) {
        out << from << " -> " << to << ";\n";
    };
    auto outputEquivalence = [&](StringView from, StringView to) {
        out << from << " -> " << to << "[style=dashed];\n";
    };

    std::map<StringId, const Pipeitem *> itemsPerOutputs;
    std::set<StringId> usedOutputs;
    std::set<StringId> itemUsedOutputs;
    std::map<StringId, const PropertySpec *> intermediateSpecs;

    for (auto [nameId, spec] : pipeline.outputSpecs) {
        usedOutputs.insert(nameId);
        intermediateSpecs[nameId] = &spec;
    }

    for (auto [nameId, spec] : pipeline.inputSpecs) {
        intermediateSpecs[nameId] = &spec;
    }

    for (auto &item : pipeline.items) {
        for (auto [nameId, localNameId] : item.outputToLocalVariables) {
            itemsPerOutputs[nameId] = &item;
            intermediateSpecs[nameId] = &item.p_task->getOutputSpecs().at(nameId);
        }
        for (auto [nameId, localNameId] : item.inputToLocalVariables) {
            usedOutputs.insert(nameId);
            itemUsedOutputs.insert(nameId);
        }
    }

    /*
    Output
    */

    out << "digraph {\n";
    out << "\trankdir=\"LR\"\n";

    // inputs
    out << "\tsubgraph cluster_inputs {\n";
    out << "\t\tlabel=\"Inputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";
    for (auto [nameId, spec] : pipeline.inputSpecs) {
        out << "\t\t";
        outputPropNode(getInputPropId(spec), spec);
    }
    out << "\t}\n";

    // outputs
    out << "\tsubgraph cluster_outputs {\n";
    out << "\t\tlabel=\"Outputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";
    for (auto [nameId, spec] : pipeline.outputSpecs) {
        out << "\t\t";
        if (pipeline.pipethroughInputNames.find(nameId) != pipeline.pipethroughInputNames.end() ||
            itemUsedOutputs.find(nameId) != itemUsedOutputs.end()) {
            outputPropNode(getOutputPropId(spec) + "_out", spec);
        } else {
            outputPropNode(getOutputPropId(spec), spec);
        }
    }
    out << "\t}\n";

    // pipethrough
    for (auto nameId : pipeline.pipethroughInputNames) {
        out << "\t";
        outputEquivalence(getInputPropId(*intermediateSpecs.at(nameId)),
                          getOutputPropId(*intermediateSpecs.at(nameId)) + "_out");
    }

    // both task inputs and pipeline outputs
    for (auto [nameId, spec] : pipeline.outputSpecs) {
        if (itemUsedOutputs.find(nameId) != itemUsedOutputs.end() &&
            pipeline.pipethroughInputNames.find(nameId) == pipeline.pipethroughInputNames.end()) {
            out << "\t";
            outputEquivalence(getOutputPropId(spec), getOutputPropId(spec) + "_out");
        }
    }

    out << "\tsubgraph cluster_processing {\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=invis;\n";

    // intermediates
    for (auto [nameId, p_spec] : intermediateSpecs) {
        if (itemUsedOutputs.find(nameId) != itemUsedOutputs.end()) {
            out << "\t\t";
            outputPropNode(getOutputPropId(*p_spec), *p_spec);
        }
    }

    // tasks
    std::size_t ordinalI = 0;
    for (auto &item : pipeline.items) {
        ++ordinalI;
        out << "\t\t";
        outputTaskNode(getTaskId(*item.p_task), ordinalI, *item.p_task);

        for (auto [nameId, localNameId] : item.outputToLocalVariables) {
            if (usedOutputs.find(nameId) != usedOutputs.end()) {
                out << "\t\t";
                outputConnection(getTaskId(*item.p_task),
                                 getOutputPropId(*intermediateSpecs.at(nameId)));
            }
        }
        for (auto [nameId, localNameId] : item.inputToLocalVariables) {
            out << "\t\t";
            outputConnection(getInputPropId(*intermediateSpecs.at(nameId)),
                             getTaskId(*item.p_task));
        }
    }
    out << "\t}\n";

    out << "}";
}

} // namespace Vitrae