/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/
// (c) 2025 ETAS GmbH. All rights reserved.
#ifndef LOGICAL_HPP_INCLUDED
#define LOGICAL_HPP_INCLUDED

#ifndef PHM_PRIVATE
#    define PHM_PRIVATE private
#endif

#include <cstdint>

#include <vector>
#include "etas/vrte/saf/common/Observer.hpp"
#include "etas/vrte/saf/common/TimeSortingBuffer.hpp"
#include "etas/vrte/saf/ifappl/Checkpoint.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/supervision/ICheckpointSupervision.hpp"
#include "etas/vrte/saf/supervision/ProcessStateTracker.hpp"
#include "etas/vrte/saf/supervision/SupervisionCfg.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifexm
{
class ProcessState;
}
namespace supervision
{

/// @brief Logical Supervision
/// @details Logical Supervision contains the logic for health monitoring - Logical supervision
/* RULECHECKER_comment(0, 38, check_source_character_set, "Special character in comment is mandatory\
    due to sphinx-need syntax.", false) */
/// @verbatim embed:rst:leading-slashes
/// The Logical Supervision class contains a sub class GraphElement to create a Checkpoint Graph.
///
/// .. uml::
///
///    class GraphElement {
///        - ifappl::Checkpoint& trackedCheckpoint_r
///        - std::vector<GraphElement*> validNextElements
///        - bool k_isFinalCheckpoint
///        + void addValidNextGraphElement(GraphElement &f_element_r)
///        + Checkpoint* getTrackedCheckpoint(void)
///        + bool isFinal(void)
///        + EResult getNextElement(GraphElement*& f_returnElement_pr, const Checkpoint& f_checkpoint_r)
///    }
///    GraphElement -> GraphElement
///
/// This way a Checkpoint Graph can be created in two steps:
///     1. Create all GraphElements (each GraphElement represents one Checkpoint)
///     2. Connect GraphElements via addValidNextGraphElement() method
///
/// Example for a configured Graph:
///
///     - :ref:`Logical graph example<logical-graph-example>`
///
/// Once a Graph is set up the transitions can then be checked via getNextGraphElement(),
/// which tries to get the next Graph position via a Checkpoint. Either the next position can be found
/// or an error occured.
///
/// The Logical Supervision state machine implementation is a combination of Adaptive Autosar
/// Logical Supervision (correct, incorrect) and Local Supervision requirements (de/activation).
/// The resulting state machine is documented here:
///
///     - :ref:`Logical Supervision - State Machine<supervision-state-machine-overview>`
///     - :ref:`Logical timing diagram for running process<logical-timing-evaluation-process-running>`
///     - :ref:`Logical timing diagram for restarted process<logical-timing-evaluation-process-restart>`
///     - :ref:`Logical timing diagram for multiple processes<logical-timing-evaluation-multi-process>`
///
/// @endverbatim
/* RULECHECKER_comment(0, 3, check_multiple_non_interface_bases, "Observable and Observer are tolerated\
    exceptions of this rule.", false) */
class Logical : public ICheckpointSupervision, public saf::common::Observable<Logical>
{
public:
    /// @brief Graph Element
    class GraphElement
    {
    public:
        /// @brief No Default Constructor
        GraphElement() = delete;

        /// @brief Constructor
        /// @param [in] f_checkpoint_r   Tracked Checkpoint
        /// @param [in] f_isFinal        Is Final Checkpoint
        GraphElement(ifappl::Checkpoint& f_checkpoint_r, const bool f_isFinal) noexcept;

        /// @brief Default Move Constructor
        /* RULECHECKER_comment(0, 3, check_incomplete_data_member_construction, "Default move constructor is\
        moving all members", false) */
        GraphElement(GraphElement&&) noexcept = default;

        /// @brief No Move Assignment
        GraphElement& operator=(GraphElement&&) = delete;
        /// @brief No Copy Constructor
        GraphElement(const GraphElement&) = delete;
        /// @brief No Copy Assignment
        GraphElement& operator=(const GraphElement&) = delete;

        /// @brief Default Destructor
        virtual ~GraphElement() = default;

        /// @brief Add a valid "next" Graph Element which represents a valid transition.
        /// @param [in] f_element_r      Graph Element to be considered as valid next Graph Element
        void addValidNextGraphElement(GraphElement& f_element_r) noexcept(false);

        ///@brief Get tracked Checkpoint
        ///@return Pointer to tracked Checkpoint
        ifappl::Checkpoint* getTrackedCheckpoint(void) const noexcept;

        /// @brief Is Final Checkpoint
        /// @return  True    Is Final Checkpoint
        ///         False   Not Final Checkpoint
        bool isFinal(void) const noexcept;

        /// @brief Result possibilities for check graph transition
        enum class EResult : uint8_t
        {
            valid = 0,       // Valid
            validFinal = 1,  // Valid and Final Checkpoint reached (end of Graph)
            error = 2,       // Error
        };

        /// @brief Get next Graph Element
        /// @param [inout] f_returnElement_pr    Pointer of next Graph Element
        /// @param [in] f_checkpoint_r           Checkpoint for Graph transition
        /// @return error                        No valid Graph transition found f_returnElement_pr not modified
        ///         valid                        Valid Graph transition found f_returnElement_pr contains next Element
        ///         validFinal                   Valid Graph transition to Final Checkpoint found f_returnElement_pr
        ///                                      contains Element which represents a Final Checkpoint
        EResult getNextElement(GraphElement*& f_returnElement_pr,
                               const ifappl::Checkpoint& f_checkpoint_r) const noexcept;

    PHM_PRIVATE:
        /// @brief Is Final Checkpoint
        const bool k_isFinalCheckpoint{false};

        /// @brief Tracked Checkpoint
        ifappl::Checkpoint& trackedCheckpoint_r;

        /// @brief Valid next Graph Elements
        std::vector<GraphElement*> validNextElements{};
    };

    /// @brief Logical Supervision Graph
    class Graph
    {
    public:
        /// @brief No Default Constructor
        Graph() = delete;

        /// @brief Constructor
        /// @details Both the f_entries_r and f_graphElements_r vector will be moved into the Graph class.
        /// @param [in] f_entries_r          Graph entry points (which reference to Initial Checkpoints)
        /// @param [in] f_graphElements_r    Full Graph
        Graph(std::vector<GraphElement*>& f_entries_r,
              std::vector<GraphElement>& f_graphElements_r) noexcept;

        /// @brief Default Move Constructor
        Graph(Graph&&) noexcept = default;

        /// @brief No Move Assignment
        Graph& operator=(Graph&&) = delete;
        /// @brief No Copy Constructor
        Graph(const Graph&) = delete;
        /// @brief No Copy Assignment
        Graph& operator=(const Graph&) = delete;

        /// @brief Default Destructor
        virtual ~Graph() = default;

        /// @brief Check if transition to given checkpoint is valid
        /// @param [in] f_checkpoint_r   Checkpoint for Graph transition
        /// @return                      True: valid transition otherwise False
        bool isValidTransition(const ifappl::Checkpoint& f_checkpoint_r) noexcept;

        /// @brief Get latest valid reported checkpoint
        /// @note May be nullptr if graph is not active, check with isActive()
        /// @return Pointer to current graph element
        const GraphElement* getCurrentGraphPosition() const noexcept;

        /// @brief Reset Graph
        /// @details Reset the Graph to initial state (inactive)
        void resetGraph(void) noexcept;

        /// @brief Check if the graph is active, i.e. a valid entrypoint was reported before
        /// @return True if graph is active, false otherwise
        bool isActive() const noexcept;

    PHM_PRIVATE:
        /// @brief Check if given checkpoint is a valid graph entry
        /// @param f_checkpoint_r   Checkpoint for graph entry
        /// @return                 True: valid entry of graph otherwise False
        bool
        isValidEntry(const ifappl::Checkpoint& f_checkpoint_r) noexcept;

        /// @brief Vector of Graph elements
        std::vector<GraphElement> elements{};

        /// @brief Vector of valid Graph entries
        std::vector<GraphElement*> entries{};

        /// @brief Graph active status
        bool isGraphActive{false};

        /// @brief Current Graph position
        GraphElement* currentGraphPosition_p{nullptr};
    };

    /// @brief Constructor
    /// @param [in] f_logicalCfg_r  Logical Supervision Configuration
    explicit Logical(const LogicalSupervisionCfg& f_logicalCfg_r) noexcept(false);

    /// @brief Default Move Constructor
    /* RULECHECKER_comment(0, 6, check_copy_in_move_constructor, "There are some const members in the subclasses\
    which leads to invocation of their appropriate copy constructors when move-construct a Logical instance",\
    true_no_defect) */
    /* RULECHECKER_comment(0, 3, check_incomplete_data_member_construction, "Default move constructor is\
    moving all members", false) */
    Logical(Logical&&) = default;

    /// @brief No Move Assignment
    Logical& operator=(Logical&&) = delete;
    /// @brief No Copy Constructor
    Logical(const Logical&) = delete;
    /// @brief No Copy Assignment
    Logical& operator=(const Logical&) = delete;

    /// @brief Destructor
    ~Logical() override = default;

    /// @brief Add Checkpoint Graph
    /// @details Both the f_graphEntries_r and f_graphElements_r vector will be moved into the Logical Supervision.
    /// @param [in] f_graphEntries_r    Graph entry points (which reference to Initial Checkpoints)
    /// @param [in] f_graphElements_r   Full Graph
    void addGraph(std::vector<GraphElement*>& f_graphEntries_r,
                  std::vector<GraphElement>& f_graphElements_r) noexcept(false);

    /// @copydoc ICheckpointSupervision::updateData(const ifappl::Checkpoint&)
    void updateData(const ifappl::Checkpoint& f_observable_r) noexcept(true) override;

    /// @copydoc ICheckpointSupervision::updateData(const ifexm::ProcessState&)
    void updateData(const ifexm::ProcessState& f_observable_r) noexcept(true) override;

    /// @copydoc ISupervision::evaluate()
    void evaluate(const timers::NanoSecondType f_syncTimestamp) override;

    /// @copydoc ICheckpointSupervision::getStatus()
    EStatus getStatus(void) const noexcept(true) override;

    /// @copydoc ICheckpointSupervision::getTimestamp()
    timers::NanoSecondType getTimestamp(void) const noexcept(true) override;

PHM_PRIVATE:
    /// @brief Check and trigger transition out of state Deactivated
    /// @param [in] f_updateEventType        Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    void
    checkTransitionsOutOfDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                        const timers::NanoSecondType f_updateEventTimestamp) noexcept;

    /// @brief Check and trigger common transitions to state Deactivated
    /// @param [in] f_updateEventType        Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    void checkTransitionsToDeactivated(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                       const timers::NanoSecondType f_updateEventTimestamp) noexcept;

    /// @brief Check and trigger recovery transition
    /// @details The recovery transition is triggered if a crashed process has been restarted (kRecoveredFromCrash)
    ///          If the recovery transition is triggered, the supervision is switched to deactivated and afterwards to
    ///          ok.
    /// @param [in] f_updateEventType       Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp  Timestamp of update event
    /// @return True: if recovery transition was triggered, False: otherwise
    bool checkForRecoveryTransition(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                    const timers::NanoSecondType f_updateEventTimestamp) noexcept(true);

    /// @brief Check and trigger transition out of state Ok
    /// @param [in] f_updateEventType        Type of update event (e.g, Activation, Deactivation, Checkpoint, ...)
    /// @param [in] f_updateEventTimestamp   Timestamp of sorted checkpoint from buffer
    /// @param [in] f_updateEvent            Update event object (e.g, Activation, Deactivation, Checkpoint, ...)
    void checkTransitionsOutOfOk(const ICheckpointSupervision::EUpdateEventType f_updateEventType,
                                 const timers::NanoSecondType f_updateEventTimestamp,
                                 const TimeSortedUpdateEvent f_updateEvent) noexcept;

    /// @brief Switch to state Deactivate
    void switchToDeactivated(void) noexcept;

    /// @brief Switch to state Ok
    void switchToOk(void) noexcept;

    /// @brief Reason for switching internal state
    enum class EReason : std::uint8_t
    {
        kDataLoss,          ///< Checkpoint data was lost due to buffer overrun
        kDataCorruption,    ///< Data corruption occured
        kInvalidTransition  ///< No valid transition to next checkpoint
    };

    /// @brief Store failure information for better error messages when going to EXPIRED state
    struct ExpiredFailureInfo
    {
        /// @brief The currently last valid reported checkpoint
        std::uint32_t currentCheckpointId{0U};
        /// @brief The newly reported invalid checkpoint
        std::uint32_t reportedCheckpointId{0U};
        /// @brief The process that reported the invalid checkpoint
        // cppcheck-suppress unusedStructMember
        const ifexm::ProcessState* processReportedInvalidCp{nullptr};
    };

    /// @brief Storing failure information when transitioning to EXPIRED
    ExpiredFailureInfo failureInfo;

    /// @brief Switch to state Expired
    /// @param [in] reason    Reason for switch to Expired
    void switchToExpired(EReason reason) noexcept;

    /// @brief Is valid Graph Transition
    /// @details Check if a given Checkpoint is a valid transition from the current
    /// @param [in] f_checkpoint_r  Checkpoint used for checking a valid Graph Transition
    /// @return True                Valid Graph Transition was found
    ///         False               Invalid Graph Transition
    bool isValidGraphTransition(const ifappl::Checkpoint& f_checkpoint_r) const noexcept;

    /// @brief Monitored Checkpoint Graph
    std::unique_ptr<Graph> graph_p{};

    /// @brief Data loss event marker
    bool isDataLossEvent{false};

    /// @brief Logical Supervision status
    EStatus logicalStatus{EStatus::deactivated};

    /// @brief Logger
    logging::PhmLogger& logger_r;

    /// @brief Timestamp in which state change is detected in [nano seconds]
    /// @details This timestamp is updated whenever assessment is done or data loss has occurred.
    saf::timers::NanoSecondType eventTimestamp{0U};

    /// @brief Sync timestamp from current evaluation [nano seconds]
    /// @details This is required for eventTimestamp in case of data loss
    saf::timers::NanoSecondType lastSyncTimestamp{0U};

    /// @brief Time sorting checkpoint buffer
    /// @details The buffer enables the logical supervision that checkpoint can be received
    /// from different Monitor interfaces in a given time frame e.g. two PHM Daemon cycles
    etas::vrte::saf::common::TimeSortingBuffer<TimeSortedUpdateEvent> timeSortingUpdateEventBuffer;

    /// @brief Keeps track of all relevant processes
    ProcessStateTracker processTracker;
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
