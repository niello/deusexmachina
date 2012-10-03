//------------------------------------------------------------------------------
//  nclodeventhandler.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "ncterrain2/nclodeventhandler.h"
#include "ncterrain2/nchunklodnode.h"

//------------------------------------------------------------------------------
/**
    This method is called when an event inside the chunk lod tree was triggered.
*/
void
nCLODEventHandler::OnEvent(Event event, nChunkLodNode* node)
{
    n_assert(node);
    switch (event)
    {       
        case CreateNode:    
            n_printf("nCLODEventHandler: CreateNode (%d, %d, %d)\n", node->GetLevel(), node->GetX(), node->GetZ());
            break;

        case DestroyNode:
            n_printf("nCLODEventHandler: DestroyNode (%d, %d, %d)\n", node->GetLevel(), node->GetX(), node->GetZ());
            break;

        case RequestLoadData:
            n_printf("nCLODEventHandler: RequestLoadData (%d, %d, %d)\n", node->GetLevel(), node->GetX(), node->GetZ());
            break;

        case DataAvailable:
            n_printf("nCLODEventHandler: DataAvailable (%d, %d, %d)\n", node->GetLevel(), node->GetX(), node->GetZ());
            break;

        case UnloadData:
            n_printf("nCLODEventHandler: UnloadData (%d, %d, %d)\n", node->GetLevel(), node->GetX(), node->GetZ());
            break;

        case RenderNode:
            // n_printf("nCLODEventHandler: RenderNode (%d, %d, %d)\n", node->GetLevel(), node->GetX(), node->GetZ());
            break;

        default:
            n_printf("nCLODEventHandler: UnknownEvent (%d, %d, %d)\n", node->GetLevel(), node->GetX(), node->GetZ());
            break;
    }
}
