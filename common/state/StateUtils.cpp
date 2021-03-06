/* Copyright 2018 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "crypto.h"
#include "error.h"
#include "jsonvalue.h"
#include "parson.h"
#include "state.h"
#include "log.h"

namespace pstate = pdo::state;

pstate::StateBlockId& pdo::state::StateNode::GetBlockId()
{
    return blockId_;
}

pstate::StateBlock& pdo::state::StateNode::GetBlock()
{
    return stateBlock_;
}

void pdo::state::StateNode::AppendChild(StateNode& childNode)
{
    try
    {
        ChildrenArray_.push_back(childNode.GetBlockId());
    }
    catch (const std::exception& e)
    {
        SAFE_LOG_EXCEPTION("StateNode::AppendChild, push_back error, out of memory");
        throw;
    }
    childNode.SetHasParent();
}

void pdo::state::StateNode::AppendChildId(StateBlockId& childId)
{
    try
    {
        ChildrenArray_.push_back(childId);
    }
    catch (const std::exception& e)
    {
        SAFE_LOG_EXCEPTION("StateNode::AppendChildId, push_back error, out of memory");
        throw;
    }
}

void pdo::state::StateNode::SetHasParent()
{
    hasParent_ = true;
}

void pdo::state::StateNode::BlockifyChildren()
{
    try
    {
        // create the master block from scratch
        stateBlock_.clear();
        // insert a JSON blob containing the BlockIds array
        JsonValue j_root_block_value(json_value_init_object());
        pdo::error::ThrowIf<pdo::error::RuntimeError>(
            !j_root_block_value.value, "Failed to create json root block value");
        JSON_Object* j_root_block_object = json_value_get_object(j_root_block_value);
        pdo::error::ThrowIfNull(
            j_root_block_object, "Failed on retrieval of json root block object value");
        JSON_Status jret;
        jret = json_object_set_value(j_root_block_object, "BlockIds", json_value_init_array());
        pdo::error::ThrowIf<pdo::error::RuntimeError>(
            jret != JSONSuccess, "failed to serialize block ids");
        JSON_Array* j_block_ids_array = json_object_get_array(j_root_block_object, "BlockIds");
        pdo::error::ThrowIfNull(j_block_ids_array, "failed to serialize the block id array");

        // insert in the array the IDs of all blocks in the list
        for (unsigned int i = 0; i < ChildrenArray_.size(); i++)
        {
            jret = json_array_append_string(
                j_block_ids_array, ByteArrayToBase64EncodedString(ChildrenArray_[i]).c_str());
        }

        //remove children blocks (reduce memory consumption)
        ChildrenArray_.resize(0);
        ChildrenArray_.shrink_to_fit();

        size_t serializedSize = json_serialization_size(j_root_block_value);

        try
        {
            stateBlock_.resize(serializedSize);
        }
        catch (const std::exception& e)
        {
            SAFE_LOG_EXCEPTION("unable to resize root git statblock for json blob");
            throw;
        }
        jret = json_serialize_to_buffer(
            j_root_block_value, reinterpret_cast<char*>(stateBlock_.data()), stateBlock_.size());
        pdo::error::ThrowIf<pdo::error::RuntimeError>(
            jret != JSONSuccess, "json root block serialization failed");
    }
    catch (const std::exception& e)
    {
        SAFE_LOG_EXCEPTION("BlockifyChildren");
        throw;
    }
}

void pdo::state::StateNode::UnBlockifyChildren()
{
    if (stateBlock_.empty())
    {
        std::string msg("Can't unblockify state node, block is empty");
        throw pdo::error::ValueError(msg);
    }
    JsonValue j_root_block_value(json_parse_string(ByteArrayToString(stateBlock_).c_str()));
    pdo::error::ThrowIf<pdo::error::RuntimeError>(
        !j_root_block_value.value, "Failed to parse json root block value");
    JSON_Object* j_root_block_object = json_value_get_object(j_root_block_value);
    pdo::error::ThrowIfNull(
        j_root_block_object, "Failed on retrieval of json root block object value");
    JSON_Array* j_block_ids_array = json_object_get_array(j_root_block_object, "BlockIds");
    pdo::error::ThrowIfNull(j_block_ids_array, "Failed to parse the block ids, expecting array");
    int block_ids_count = json_array_get_count(j_block_ids_array);
    for (int i = 0; i < block_ids_count; i++)
    {
        try
        {
            ChildrenArray_.push_back(StateBlockId(
                Base64EncodedStringToByteArray(json_array_get_string(j_block_ids_array, i))));
        }
        catch (const std::exception& e)
        {
            SAFE_LOG_EXCEPTION("error allocating children in state node");
            throw;
        }
    }
}

pstate::StateBlockIdArray pdo::state::StateNode::GetChildrenBlocks()
{
    return ChildrenArray_;
}

void pdo::state::StateNode::ClearChildren()
{
    ChildrenArray_.clear();
}
