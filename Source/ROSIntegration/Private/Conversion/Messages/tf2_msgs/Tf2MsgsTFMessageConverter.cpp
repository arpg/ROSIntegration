#include "Conversion/Messages/tf2_msgs/Tf2MsgsTFMessageConverter.h"


UTf2MsgsTFMessageConverter::UTf2MsgsTFMessageConverter()
{
	_MessageType = "tf2_msgs/TFMessage";
}

bool UTf2MsgsTFMessageConverter::ConvertIncomingMessage(const ROSBridgePublishMsg* message, TSharedPtr<FROSBaseMsg> &BaseMsg) 
{
    auto msg = new ROSMessages::tf2_msgs::TFMessage;
    BaseMsg = TSharedPtr<FROSBaseMsg>(msg);
	return _bson_extract_child_tf2_msg(message->full_msg_bson_, "msg", msg);
}


bool UTf2MsgsTFMessageConverter::ConvertOutgoingMessage(TSharedPtr<FROSBaseMsg> BaseMsg, bson_t** message) 
{
    auto CastMsg = StaticCastSharedPtr<ROSMessages::tf2_msgs::TFMessage>(BaseMsg);
    if (!CastMsg)
    {
        UE_LOG(LogTemp, Error, TEXT("UTf2MsgsTFMessageConverter::ConvertOutgoingMessage - Cast to TFMessage failed"));
        return false;
    }

    if (CastMsg->transforms.Num() == 0) 
    {
        UE_LOG(LogTemp, Warning, TEXT("UTf2MsgsTFMessageConverter::ConvertOutgoingMessage - No transform saved in TFMessage. Can't convert message"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("UTf2MsgsTFMessageConverter::ConvertOutgoingMessage - Preparing to convert TFMessage with %d transforms"), CastMsg->transforms.Num());

    // Initialize a new BSON message
    *message = bson_new();
    if (*message == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("UTf2MsgsTFMessageConverter::ConvertOutgoingMessage - Failed to create new BSON message"));
        return false;
    }

    _bson_append_tf2_msg(*message, CastMsg.Get());

    UE_LOG(LogTemp, Log, TEXT("UTf2MsgsTFMessageConverter::ConvertOutgoingMessage - TFMessage converted to BSON successfully"));

    return true;
}


// bool UTf2MsgsTFMessageConverter::ConvertOutgoingMessage(TSharedPtr<FROSBaseMsg> BaseMsg, bson_t** message) 
// {
// 	auto CastMsg = StaticCastSharedPtr<ROSMessages::tf2_msgs::TFMessage>(BaseMsg);
// 	if (CastMsg->transforms.Num() == 0) 
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("No transform saved in TFMessage. Can't convert message"));
// 		return false;
// 	}
// 	_bson_append_tf2_msg(*message, CastMsg.Get());
// 	return true;
// }