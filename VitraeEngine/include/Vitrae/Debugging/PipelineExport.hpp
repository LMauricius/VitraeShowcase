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

    auto escapedLabel = [&](StringView label) -> String {
        String ret = searchAndReplace(String(label), "\"", "\\\"");
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
        return String("Task_") + std::to_string((std::size_t)&task);
    };
    auto outputTaskNode = [&](StringView id, std::size_t ord, const BasicTask &task) {
        out << id << " [";
        out << "label=\""
            << escapedLabel(String("#") + std::to_string(ord) + "\\n" +
                            String(task.getFriendlyName()))
            << "\", ";
        out << "group=\"Tasks\", ";
        out << "shape=box, ";
        out << "style=\"filled\", ";
        out << "fillcolor=\"lightblue\", ";
        out << "bgcolor=\"lightblue\", ";
        out << "];\n";
    };
    auto outputPropNode = [&](StringView id, const PropertySpec &spec) {
        out << id << " [";
        out << "label=\"" << escapedLabel(spec.name);
        out << "\\n: " << escapedLabel(spec.typeInfo.getShortTypeName()) << "\", ";
        out << "shape=box, ";
        out << "style=\"rounded,filled\", ";
        out << "fillcolor=\"lightyellow\", ";
        out << "bgcolor=\"lightyellow\", ";
        out << "];\n";
    };
    auto outputInvisNode = [&](StringView id) {
        out << id << " [";
        out << "style=\"invis\", ";
        out << "shape=\"point\", ";
        out << "];\n";
    };
    auto sameRank = [&](StringView from, StringView to) {
        out << "{rank=same; " << from << "; " << to << "}\n";
    };
    auto outputConnection = [&](StringView from, StringView to, bool changeRank) {
        out << from << " -> " << to;
        if (!changeRank) {
            out << " [minlen=0]";
        }
        out << ";\n";
    };
    auto outputEquivalence = [&](StringView from, StringView to, bool changeRank,
                                 StringView lhead = "", StringView ltail = "") {
        out << from << " -> " << to;
        out << "[style=dashed";
        if (lhead.length()) {
            out << ", lhead=" << lhead;
        }
        if (ltail.length()) {
            out << ", ltail=" << ltail;
        }
        if (!changeRank) {
            out << ", minlen=0";
        }
        out << "];\n";
    };
    auto outputInvisibleDirection = [&](StringView from, StringView to, bool changeRank,
                                        StringView lhead = "", StringView ltail = "") {
        out << from << " -> " << to << "[style=invis";
        if (lhead.length()) {
            out << ", lhead=" << lhead;
        }
        if (ltail.length()) {
            out << ", ltail=" << ltail;
        }
        if (!changeRank) {
            out << ", minlen=0";
        }
        out << "];\n";
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

    bool horizontalInputsOutputs = pipeline.inputSpecs.size() > pipeline.items.size() * 2.5 ||
                                   pipeline.outputSpecs.size() > pipeline.items.size() * 2.5;
    horizontalInputsOutputs = false;

    /*
    Output
    */

    out << "digraph {\n";
    out << "\trankdir=\"LR\"\n";
    out << "\tranksep=0.02\n";
    out << "\tcompound=true;\n";

    // inputs
    out << "\tsubgraph cluster_inputs {\n";
    out << "\t\tlabel=\"Inputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";

    out << "\t\t";
    outputInvisNode("cluster_inputs_node");

    for (auto [nameId, spec] : pipeline.inputSpecs) {
        out << "\t\t";
        outputPropNode(getInputPropId(spec), spec);
    }
    out << "\t}\n";

    out << "\tsubgraph cluster_processing {\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=invis;\n";

    out << "\t\t";
    outputInvisNode("cluster_processing_node");

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

        for (auto [nameId, localNameId] : item.inputToLocalVariables) {
            out << "\t";
            outputConnection(getInputPropId(*intermediateSpecs.at(nameId)), getTaskId(*item.p_task),
                             !horizontalInputsOutputs ||
                                 pipeline.inputSpecs.find(nameId) == pipeline.inputSpecs.end());
        }
    }
    out << "\t}\n";

    // outputs
    out << "\tsubgraph cluster_outputs {\n";
    out << "\t\tlabel=\"Outputs\";\n";
    out << "\t\tcluster=true;\n";
    out << "\t\tstyle=dashed;\n";

    out << "\t\t";
    outputInvisNode("cluster_outputs_node");

    for (auto [nameId, spec] : pipeline.outputSpecs) {
        out << "\t\t";
        if (pipeline.pipethroughInputNames.find(nameId) != pipeline.pipethroughInputNames.end() ||
            itemUsedOutputs.find(nameId) != itemUsedOutputs.end()) {
            outputPropNode(getOutputPropId(spec) + "_out", spec);
        } else {
            outputPropNode(getOutputPropId(spec), spec);
        }
    }

    // processed properties
    for (auto &item : pipeline.items) {
        for (auto [nameId, localNameId] : item.outputToLocalVariables) {
            if (usedOutputs.find(nameId) != usedOutputs.end()) {
                out << "\t\t";
                outputConnection(
                    getTaskId(*item.p_task), getOutputPropId(*intermediateSpecs.at(nameId)),
                    !horizontalInputsOutputs ||
                        pipeline.outputSpecs.find(nameId) == pipeline.outputSpecs.end() ||
                        itemUsedOutputs.find(nameId) != itemUsedOutputs.end());
            }
        }
    }

    out << "\t}\n";

    // pipethrough
    for (auto nameId : pipeline.pipethroughInputNames) {
        out << "\t";
        outputEquivalence(getInputPropId(*intermediateSpecs.at(nameId)),
                          getOutputPropId(*intermediateSpecs.at(nameId)) + "_out", false);
    }

    // both task inputs and pipeline outputs
    for (auto [nameId, spec] : pipeline.outputSpecs) {
        if (itemUsedOutputs.find(nameId) != itemUsedOutputs.end() &&
            pipeline.pipethroughInputNames.find(nameId) == pipeline.pipethroughInputNames.end()) {
            out << "\t";
            outputEquivalence(getOutputPropId(spec), getOutputPropId(spec) + "_out", false);
        }
    }

    // order clusters
    /*if (pipeline.inputSpecs.size() && pipeline.items.size()) {
        outputInvisibleDirection(getInputPropId((*pipeline.inputSpecs.begin()).second),
                                 getTaskId(*pipeline.items.front().p_task), false, "cluster_inputs",
                                 "cluster_processing");
    }*/
    outputInvisibleDirection("cluster_inputs_node", "cluster_processing_node", false,
                             "cluster_inputs", "cluster_processing");
    /*if (pipeline.items.size() && pipeline.outputSpecs.size()) {
        String outNodeId;
        auto &outNodeSpec = (*pipeline.outputSpecs.begin()).second;

        if (pipeline.pipethroughInputNames.find(outNodeSpec.name) !=
                pipeline.pipethroughInputNames.end() ||
            itemUsedOutputs.find(outNodeSpec.name) != itemUsedOutputs.end()) {
            outNodeId = getOutputPropId(outNodeSpec) + "_out";
        } else {
            outNodeId = getOutputPropId(outNodeSpec);
        }
        outputInvisibleDirection(getTaskId(*pipeline.items.front().p_task), outNodeId, false,
                                 "cluster_processing", "cluster_outputs");
    }*/
    outputInvisibleDirection("cluster_processing_node", "cluster_outputs_node", true,
                             "cluster_processing", "cluster_outputs");

    // for many inputs/outputs we lay them in the opposite direction to the graph
    // order them by appearence
    if (horizontalInputsOutputs) {
        String lastInputNodeId = "cluster_inputs_node";
        String lastOutputNodeId = "";

        std::set<StringId> processedNodes;
        auto processInputNode = [&](const String &nodeId) {
            if (processedNodes.find(nodeId) == processedNodes.end()) {
                if (lastInputNodeId.length()) {
                    out << "\t\t";
                    outputInvisibleDirection(lastInputNodeId, nodeId, true);
                }
                processedNodes.insert(nodeId);
                lastInputNodeId = nodeId;
            }
        };
        auto processOutputNode = [&](const String &nodeId) {
            if (processedNodes.find(nodeId) == processedNodes.end()) {
                if (lastOutputNodeId.length()) {
                    out << "\t\t";
                    outputInvisibleDirection(lastOutputNodeId, nodeId, true);
                }
                processedNodes.insert(nodeId);
                lastOutputNodeId = nodeId;
            }
        };

        for (auto nameId : pipeline.pipethroughInputNames) {
            processInputNode(getInputPropId(*intermediateSpecs.at(nameId)));
            processOutputNode(getOutputPropId(*intermediateSpecs.at(nameId)) + "_out");
            out << "\t\t";
            sameRank(getInputPropId(*intermediateSpecs.at(nameId)),
                     getOutputPropId(*intermediateSpecs.at(nameId)) + "_out");
        }

        for (auto &item : pipeline.items) {
            for (auto [nameId, localNameId] : item.outputToLocalVariables) {
                if (pipeline.outputSpecs.find(nameId) != pipeline.outputSpecs.end()) {
                    if (itemUsedOutputs.find(nameId) != itemUsedOutputs.end()) {
                        processOutputNode(getOutputPropId(*intermediateSpecs.at(nameId)) + "_out");
                    } else {
                        processOutputNode(getOutputPropId(*intermediateSpecs.at(nameId)));
                    }
                }
            }
            for (auto [nameId, localNameId] : item.inputToLocalVariables) {
                if (pipeline.inputSpecs.find(nameId) != pipeline.inputSpecs.end()) {
                    processInputNode(getInputPropId(*intermediateSpecs.at(nameId)));
                }
            }
        }

        processOutputNode("cluster_outputs_node");

        out << "\t\tnewrank=true;\n";
    }

    out << "}";
}

} // namespace Vitrae