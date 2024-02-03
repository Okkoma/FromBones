#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>

#include "GameContext.h"
#include "GameEvents.h"
#include "GameNetwork.h"

#include "DefsNetwork.h"

const char* gameStatusNames[] =
{
    "MENUSTATE=0",
    "PLAYSTATE_INITIALIZING=1",
    "PLAYSTATE_LOADING=2",
    "PLAYSTATE_FINISHLOADING=3",
    "PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS=4",
    "PLAYSTATE_CLIENT_LOADING_REPLICATEDNODES=5",
    "PLAYSTATE_SYNCHRONIZING=6",
    "PLAYSTATE_READY=7",
    "PLAYSTATE_STARTGAME=8",
    "PLAYSTATE_RUNNING=9",
    "PLAYSTATE_ENDGAME=10",
    "PLAYSTATE_WINGAME=11"
    "NOGAMESTATE=-1",
    "KILLCLIENTS=-2",
};

const char* netCommandNames[] =
{
    "NOACTION=0",
    "GAMESTATUS=1",
    "ERASENODE=2",
    "ENABLENODE=3",
    "DISABLECLIENTOBJECTCONTROL=4",
    "SETSEEDTIME=5",
    "REQUESTMAP=6",

    "ADDNODE=7",
    "EXPLODENODE=8",
    "ADDFURNITURE=9",
    "ADDCOLLECTABLE=10",

    "TRANSFERITEM=11",
    "DROPITEM=12",
    "SETITEM=13",
    "UPDATEEQUIPMENT=14",
    "SETFULLEQUIPMENT=15",
    "SETFULLINVENTORY=16",

    "CHANGETILE=17",

    "SETWEATHER=18",
    "SETMAPDATAS=19",
    "SETWORLD=20",

    "TRIGCLICKED=21",
    "ENTITYSELECTED=22",
    "MOUNTENTITYON=23",

    "UPDATEITEMSSTORAGE=24",
    "UPDATEZONEDATA=25",

    0
};

#ifdef ACTIVE_PACKEDOBJECTCONTROL

/// Float Conversion 32bits <=> 16bits
// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits

inline unsigned int as_uint(const float x) { return *(unsigned int*)&x; }
inline float as_float(const unsigned int x) { return *(float*)&x; }

// from float16 to float32
float half_to_float(const unsigned short x)
{
    const unsigned int e = (x&0x7C00)>>10; // exponent
    const unsigned int m = (x&0x03FF)<<13; // mantissa
    const unsigned int v = as_uint((float)m)>>23; // evil log2 bit hack to count leading zeros in denormalized format
    return as_float((x&0x8000)<<16 | (e!=0)*((e+112)<<23|m) | ((e==0)&(m!=0))*((v-37)<<23|((m<<(150-v))&0x007FE000))); // sign : normalized : denormalized
}

// from float32 to float16
unsigned short float_to_half(const float x)
{
    const unsigned int b = as_uint(x)+0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
    const unsigned int e = (b&0x7F800000)>>23; // exponent
    const unsigned int m = b&0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
    return (b&0x80000000)>>16 | (e>112)*((((e-112)<<10)&0x7C00)|m>>13) | ((e<113)&(e>101))*((((0x007FF000+m)>>(125-e))+1)>>1) | (e>143)*0x7FFF; // sign : normalized : denormalized : saturate
}

/// ObjectControl

void ObjectControl::Pack(PackedObjectControl& packedobject)
{
    packedobject.physics_.positionx_ = float_to_half(physics_.positionx_);
    packedobject.physics_.positiony_ = float_to_half(physics_.positiony_);
    packedobject.physics_.velx_ = float_to_half(physics_.velx_);
    packedobject.physics_.vely_ = float_to_half(physics_.vely_);
    packedobject.physics_.rotation_ = float_to_half(physics_.rotation_);
    packedobject.physics_.direction_ = float_to_half(physics_.direction_);

    packedobject.states_.type_ = states_.type_;
    packedobject.states_.spawnid_ = states_.spawnid_;
    packedobject.states_.buttons_ = states_.buttons_;
    packedobject.states_.animstateindex_ = states_.animstateindex_;
    packedobject.states_.animversion_ = states_.animversion_;
    packedobject.states_.entityid_ = states_.entityid_;
    packedobject.states_.viewZ_ = states_.viewZ_;
    packedobject.states_.flag_ = states_.flag_;
    packedobject.states_.stamp_ = states_.stamp_;
    packedobject.states_.totalDpsReceived_ = float_to_half(states_.totalDpsReceived_);

    packedobject.holderinfo_.id_ = holderinfo_.id_;
    packedobject.holderinfo_.point1x_ = float_to_half(holderinfo_.point1x_);
    packedobject.holderinfo_.point1y_ = float_to_half(holderinfo_.point1y_);
    packedobject.holderinfo_.point2x_ = float_to_half(holderinfo_.point2x_);
    packedobject.holderinfo_.point2y_ = float_to_half(holderinfo_.point2y_);
    packedobject.holderinfo_.rot2_ = float_to_half(holderinfo_.rot2_);
}

void ObjectControl::Unpack(const PackedObjectControl& packedobject)
{
    physics_.positionx_ = half_to_float(packedobject.physics_.positionx_);
    physics_.positiony_ = half_to_float(packedobject.physics_.positiony_);
    physics_.velx_ = half_to_float(packedobject.physics_.velx_);
    physics_.vely_ = half_to_float(packedobject.physics_.vely_);
    physics_.rotation_ = half_to_float(packedobject.physics_.rotation_);
    physics_.direction_ = half_to_float(packedobject.physics_.direction_);

    states_.type_ = packedobject.states_.type_ ;
    states_.spawnid_ = packedobject.states_.spawnid_;
    states_.buttons_ = packedobject.states_.buttons_;
    states_.animstateindex_ = packedobject.states_.animstateindex_;
    states_.animversion_ = packedobject.states_.animversion_;
    states_.entityid_ = packedobject.states_.entityid_;
    states_.viewZ_ = packedobject.states_.viewZ_;
    states_.flag_ = packedobject.states_.flag_;
    states_.stamp_ = packedobject.states_.stamp_;
    states_.totalDpsReceived_ = half_to_float(packedobject.states_.totalDpsReceived_);

    holderinfo_.id_ = packedobject.holderinfo_.id_;
    holderinfo_.point1x_ = half_to_float(packedobject.holderinfo_.point1x_);
    holderinfo_.point1y_ = half_to_float(packedobject.holderinfo_.point1y_);
    holderinfo_.point2x_ = half_to_float(packedobject.holderinfo_.point2x_);
    holderinfo_.point2y_ = half_to_float(packedobject.holderinfo_.point2y_);
    holderinfo_.rot2_ = half_to_float(packedobject.holderinfo_.rot2_);
}
#endif

void ObjectControl::SetFlagBit(int bit, bool state)
{
    unsigned char flag = (1 << bit);

    if (state)
        states_.flag_ |= flag;
    else
        states_.flag_ &= ~flag;
}

void ObjectControl::SetAnimationState(const StringHash& state)
{
    states_.animstateindex_ = GOS::GetStateIndex(state);
}

StringHash ObjectControl::GetAnimationState() const
{
    return GOS::GetStateFromIndex(states_.animstateindex_);
}

/// ObjectControlInfo

const ObjectControlInfo ObjectControlInfo::EMPTY;

ObjectControlInfo::ObjectControlInfo(const ObjectControlInfo& c)
{
    memcpy(this, &c, sizeof(ObjectControlInfo));
}

void ObjectControlInfo::Clear()
{
    active_= prepared_ = false;
#ifdef ACTIVE_PACKEDOBJECTCONTROL
    packed_ = false;
#endif
    serverNodeID_ = clientNodeID_ = 0;
    memset(receivedControls_, 0, 2 * sizeof(ObjectControl));
    memset(&preparedControl_, 0, sizeof(ObjectControl));
    sendedControls_.Clear();
}

void ObjectControlInfo::SetActive(bool active, bool serverobject)
{
    if (active_ != active)
    {
        if (!active && !GameContext::Get().IsAvatarNodeID(serverNodeID_))// && clientNodeID_ != serverNodeID_)
        {
            if (serverobject)
                clientNodeID_ = 0;
            else
                serverNodeID_ = 0;
        }

        active_ = active;

//        URHO3D_LOGERRORF("ObjectControlInfo() - SetActive : this=%u servernodeid=%u clientnodeid=%u actived=%s !", this, serverNodeID_, clientNodeID_, active_?"true":"false");
    }
}

void ObjectControlInfo::SetEnable(bool enable)
{
    preparedControl_.SetFlagBit(OFB_ENABLED, enable);
    receivedControls_[0].SetFlagBit(OFB_ENABLED, enable);
    receivedControls_[1].SetFlagBit(OFB_ENABLED, enable);
}

void ObjectControlInfo::BackupReceivedControl()
{
    memcpy(&receivedControls_[1], &receivedControls_[0], sizeof(ObjectControl));
}

void ObjectControlInfo::CopyReceivedControlTo(ObjectControl& ocontrol) const
{
    memcpy(&ocontrol, &receivedControls_[0], sizeof(ObjectControl));
}

void ObjectControlInfo::CopyReceivedControlAckTo(ObjectControl& ocontrol) const
{
    ocontrol.states_.totalDpsReceived_ = receivedControls_[0].states_.totalDpsReceived_;
}

void ObjectControlInfo::CopyPreparedControlTo(ObjectControl& ocontrol)
{
    memcpy(&ocontrol, &preparedControl_, sizeof(ObjectControl));
}

void ObjectControlInfo::CopyPreparedToSendedControl()
{
    sendedControls_.Resize(sendedControls_.Size()+1);
    memcpy(&sendedControls_.Back(), &preparedControl_, sizeof(ObjectControl));
}

void ObjectControlInfo::CopyPreparedPositionToInitial()
{
    preparedControl_.holderinfo_.point1x_ = preparedControl_.physics_.positionx_;
    preparedControl_.holderinfo_.point1y_ = preparedControl_.physics_.positiony_;
    preparedControl_.holderinfo_.point2x_ = preparedControl_.physics_.velx_;
    preparedControl_.holderinfo_.point2y_ = preparedControl_.physics_.vely_;
    preparedControl_.holderinfo_.rot2_ = preparedControl_.physics_.rotation_;
}

void ObjectControlInfo::UseInitialPosition(const ObjectControl& refcontrol)
{
    ObjectControl& control = receivedControls_[0];
    control.physics_.positionx_ = preparedControl_.holderinfo_.point1x_ = refcontrol.holderinfo_.point1x_;
    control.physics_.positiony_ = preparedControl_.holderinfo_.point1y_ = refcontrol.holderinfo_.point1y_;
    control.physics_.velx_ = preparedControl_.holderinfo_.point2x_ = refcontrol.holderinfo_.point2x_;
    control.physics_.vely_ = preparedControl_.holderinfo_.point2y_ = refcontrol.holderinfo_.point2y_;
    control.physics_.rotation_ = preparedControl_.holderinfo_.rot2_ = refcontrol.holderinfo_.rot2_;
    control.physics_.direction_ = control.physics_.velx_ > 0.f;
}

const ObjectControl& ObjectControlInfo::GetReceivedControl() const
{
    return receivedControls_[0];
}

const ObjectControl& ObjectControlInfo::GetPreviousReceivedControl() const
{
    return receivedControls_[1];
}

ObjectControl& ObjectControlInfo::GetReceivedControl()
{
    return receivedControls_[0];
}

ObjectControl& ObjectControlInfo::GetPreparedControl()
{
    return preparedControl_;
}

void ObjectControlInfo::Read(Deserializer& msg)
{
#ifdef ACTIVE_PACKEDOBJECTCONTROL
    msg.Read(&receivedPackedControl_, sizeof(PackedObjectControl));
    receivedControls_[0].Unpack(receivedPackedControl_);
#else
    msg.Read(&receivedControls_[0], sizeof(ObjectControl));
#endif
}

void ObjectControlInfo::ReadAck(Deserializer& msg)
{
#ifdef ACTIVE_PACKEDOBJECTCONTROL
    receivedControls_[0].states_.totalDpsReceived_ = half_to_float(msg.ReadUShort());
#else
    receivedControls_[0].states_.totalDpsReceived_ = msg.ReadFloat();
#endif
}

// Write preparedControl_ to msg
bool ObjectControlInfo::Write(Serializer& msg)
{
    msg.WriteUByte(OBJECTCONTROL);

#ifdef ACTIVE_SHORTHEADEROBJECTCONTROL
    msg.WriteUByte(clientId_);
    msg.WriteUShort((unsigned short int)(serverNodeID_ ? serverNodeID_-FIRST_LOCAL_ID : 0));
    msg.WriteUShort((unsigned short int)(clientNodeID_ ? clientNodeID_-FIRST_LOCAL_ID : 0));
#else
    msg.WriteInt(clientId_);
    msg.WriteUInt(serverNodeID_);
    msg.WriteUInt(clientNodeID_);
#endif

#ifdef ACTIVE_PACKEDOBJECTCONTROL
    if (!packed_)
    {
        preparedControl_.Pack(preparedPackedControl_);
        packed_ = true;
    }
    msg.Write(&preparedPackedControl_, sizeof(PackedObjectControl));
#else
    msg.Write(&preparedControl_, sizeof(ObjectControl));
#endif
//    URHO3D_LOGINFOF("ObjectControlInfo() - WriteMessage : nodeid=%u !", serverNodeID_);

    return true;
}

bool ObjectControlInfo::WriteAck(Serializer& msg)
{
    msg.WriteUByte(OBJECTCONTROL);

#ifdef ACTIVE_SHORTHEADEROBJECTCONTROL
    msg.WriteUByte(clientId_);
    msg.WriteUShort((unsigned short int)(serverNodeID_ ? serverNodeID_-FIRST_LOCAL_ID : 0));
    msg.WriteUShort((unsigned short int)(clientNodeID_ ? clientNodeID_-FIRST_LOCAL_ID : 0));
#else
    msg.WriteInt(clientId_);
    msg.WriteUInt(serverNodeID_);
    msg.WriteUInt(clientNodeID_);
#endif

#ifdef ACTIVE_PACKEDOBJECTCONTROL
    msg.WriteUShort(float_to_half(preparedControl_.states_.totalDpsReceived_));
#else
    msg.WriteFloat(preparedControl_.states_.totalDpsReceived_);
#endif

    return true;
}

void ObjectControlInfo::Dump() const
{
    URHO3D_LOGINFOF("ObjectControlInfo() - Dump : ptr=%u clientid=%u nodeid=(%u,%u) node=%s(%u) active=%s !",
                    this, clientId_, serverNodeID_, clientNodeID_, node_ ? node_->GetName().CString() : "NA", node_ ? node_->GetID() : 0, active_?"true":"false");
}


/// Object Command

const ObjectCommand ObjectCommand::EMPTY = ObjectCommand();

void ObjectCommand::Read(Deserializer& msg)
{
    clientId_ = msg.ReadInt();
    broadCast_ = msg.ReadBool();
    cmd_ = msg.ReadVariantMap();
}

void ObjectCommand::Write(Serializer& msg, int cmdid, int toclient) const
{
    msg.WriteUByte(OBJECTCOMMAND);
    if (cmdid != -1)
        msg.WriteUByte(cmdid);
    msg.WriteInt(clientId_);
    msg.WriteBool(broadCast_);
    msg.WriteVariantMap(cmd_);
}

void ObjectCommand::CopyTo(ObjectCommand& cmd) const
{
    cmd.clientId_ = clientId_;
    cmd.broadCast_ = broadCast_;
    cmd.cmd_ = cmd_;
}

void ObjectCommand::Dump() const
{
    int cmd = cmd_.Find(Net_ObjectCommand::P_COMMAND)->second_.GetInt();
    unsigned nodeid = cmd_.Find(Net_ObjectCommand::P_NODEID)->second_.GetUInt();

    URHO3D_LOGINFOF("ObjectCommand() - Dump : cmd=%s(%d) nodeid=%u clientid=%d broadcast=%s !",
                    cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", cmd, nodeid, clientId_, broadCast_?"true":"false");
}
