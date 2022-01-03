#include "CableView.h"
#include "../BoardComponent.h"
#include "CableDrawingHelpers.h"
#include "processors/chain/ProcessorChainActionHelper.h"

namespace
{
Point<int> getPortLocation (ProcessorEditor* editor, int portIdx, bool isInput)
{
    auto portLocation = editor->getPortLocation (portIdx, isInput);
    return portLocation + editor->getBounds().getTopLeft();
}

std::unique_ptr<Cable> connectionToCable (const ConnectionInfo& connection)
{
    return std::make_unique<Cable> (connection.startProc, connection.startPort, connection.endProc, connection.endPort);
}

ConnectionInfo cableToConnection (const Cable& cable)
{
    return { cable.startProc, cable.startIdx, cable.endProc, cable.endIdx };
}

void updateConnectionStatuses (const BoardComponent* board, const ConnectionInfo& connection, bool isConnected)
{
    if (auto* editor = board->findEditorForProcessor (connection.startProc))
    {
        bool shouldBeConnected = isConnected || connection.startProc->getNumOutputConnections (connection.startPort) > 0;
        editor->setConnectionStatus (shouldBeConnected, connection.startPort, false);
    }

    if (auto* editor = board->findEditorForProcessor (connection.endProc))
        editor->setConnectionStatus (isConnected, connection.endPort, true);
}

void addConnectionsForProcessor (OwnedArray<Cable>& cables, BaseProcessor* proc, const BoardComponent* board)
{
    for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
    {
        auto numConnections = proc->getNumOutputConnections (portIdx);
        for (int cIdx = 0; cIdx < numConnections; ++cIdx)
        {
            const auto& connection = proc->getOutputConnection (portIdx, cIdx);
            cables.add (connectionToCable (connection));

            updateConnectionStatuses (board, connection, true);
        }
    }
}
} // namespace

CableView::CableView (const BoardComponent* comp) : board (comp)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (36);
}

void CableView::paint (Graphics& g)
{
    using namespace CableConstants;
    using namespace CableDrawingHelpers;

    g.setColour (cableColour.brighter (0.1f));
    for (auto* cable : cables)
    {
        auto* startEditor = board->findEditorForProcessor (cable->startProc);
        jassert (startEditor != nullptr);

        auto startPortLocation = getPortLocation (startEditor, cable->startIdx, false);

        if (cable->endProc != nullptr)
        {
            auto* endEditor = board->findEditorForProcessor (cable->endProc);
            jassert (endEditor != nullptr);

            auto endPortLocation = getPortLocation (endEditor, cable->endIdx, true);

            auto startColour = startEditor->getColour();
            auto endColour = endEditor->getColour();
            auto levelDB = cable->endProc->getInputLevelDB (cable->endIdx);
            drawCable (g, startPortLocation.toFloat(), startColour, endPortLocation.toFloat(), endColour, scaleFactor, levelDB);
        }
        else if (cableMouse != nullptr)
        {
            auto mousePos = cableMouse->getPosition();
            auto [editor, portIdx] = getNearestInputPort (mousePos);
            if (editor != nullptr)
            {
                auto endPortLocation = getPortLocation (editor, portIdx, true);
                drawCablePortGlow (g, endPortLocation);
            }

            auto colour = startEditor->getColour();
            drawCable (g, startPortLocation.toFloat(), colour, mousePos.toFloat(), colour, scaleFactor);
        }
    }
}

void CableView::setScaleFactor (float newScaleFactor)
{
    scaleFactor = newScaleFactor;
    repaint();
}

void CableView::createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e)
{
    cables.add (std::make_unique<Cable> (origin->getProcPtr(), portIndex));
    cableMouse = std::make_unique<MouseEvent> (std::move (e.getEventRelativeTo (this)));
    repaint();
}

void CableView::refreshCable (const MouseEvent& e)
{
    cableMouse = std::make_unique<MouseEvent> (std::move (e.getEventRelativeTo (this)));
    repaint();
}

void CableView::releaseCable (const MouseEvent& e)
{
    cableMouse.reset();

    // check if we're releasing near an output port
    auto relMouse = e.getEventRelativeTo (this);
    auto mousePos = relMouse.getPosition();

    auto [editor, portIdx] = getNearestInputPort (mousePos);
    if (editor != nullptr)
    {
        auto* cable = cables.getLast();
        cable->endProc = editor->getProcPtr();
        cable->endIdx = portIdx;

        const ScopedValueSetter<bool> svs (ignoreConnectionCallbacks, true);
        auto connection = cableToConnection (*cable);
        board->procChain.getActionHelper().addConnection (std::move (connection));

        repaint();
        return;
    }

    // not being connected... trash the latest cable
    cables.removeObject (cables.getLast());

    repaint();
}

void CableView::destroyCable (ProcessorEditor* origin, int portIndex)
{
    const auto* proc = origin->getProcPtr();
    for (auto* cable : cables)
    {
        if (cable->endProc == proc && cable->endIdx == portIndex)
        {
            const ScopedValueSetter<bool> svs (ignoreConnectionCallbacks, true);
            board->procChain.getActionHelper().removeConnection (cableToConnection (*cable));
            cables.removeObject (cable);

            break;
        }
    }

    repaint();
}

std::pair<ProcessorEditor*, int> CableView::getNearestInputPort (const Point<int>& pos) const
{
    auto result = std::make_pair<ProcessorEditor*, int> (nullptr, 0);
    int minDistance = -1;

    auto checkPorts = [&] (ProcessorEditor* editor)
    {
        int numPorts = editor->getProcPtr()->getNumInputs();
        for (int i = 0; i < numPorts; ++i)
        {
            auto portLocation = getPortLocation (editor, i, true);
            auto distanceFromPort = pos.getDistanceFrom (portLocation);

            bool isClosest = (distanceFromPort < CableConstants::portDistanceLimit && minDistance < 0) || distanceFromPort < minDistance;
            if (isClosest)
            {
                minDistance = distanceFromPort;
                result = std::make_pair (editor, i);
            }
        }
    };

    for (auto* editor : board->processorEditors)
        checkPorts (editor);

    checkPorts (board->outputEditor.get());

    if (result.first == nullptr)
        return result;

    for (auto* cable : cables)
    {
        // the closest port is already connected!
        if (cable->endProc == result.first->getProcPtr() && cable->endIdx == result.second)
            return std::make_pair<ProcessorEditor*, int> (nullptr, 0);
    }

    return result;
}

void CableView::processorBeingAdded (BaseProcessor* newProc)
{
    addConnectionsForProcessor (cables, newProc, board);
}

void CableView::processorBeingRemoved (const BaseProcessor* proc)
{
    for (int i = cables.size() - 1; i >= 0; --i)
    {
        if (cables[i]->startProc == proc || cables[i]->endProc == proc)
        {
            updateConnectionStatuses (board, cableToConnection (*cables[i]), false);
            cables.remove (i);
        }
    }
}

void CableView::refreshConnections()
{
    cables.clear();

    for (auto* proc : board->procChain.getProcessors())
        addConnectionsForProcessor (cables, proc, board);

    addConnectionsForProcessor (cables, &board->procChain.getInputProcessor(), board);

    repaint();
}

void CableView::connectionAdded (const ConnectionInfo& info)
{
    updateConnectionStatuses (board, info, true);

    if (ignoreConnectionCallbacks)
        return;

    cables.add (connectionToCable (info));

    repaint();
}

void CableView::connectionRemoved (const ConnectionInfo& info)
{
    updateConnectionStatuses (board, info, false);

    if (ignoreConnectionCallbacks)
        return;

    for (auto* cable : cables)
    {
        if (cable->startProc == info.startProc
            && cable->startIdx == info.startPort
            && cable->endProc == info.endProc
            && cable->endIdx == info.endPort)
        {
            cables.removeObject (cable);
            break;
        }
    }

    repaint();
}
