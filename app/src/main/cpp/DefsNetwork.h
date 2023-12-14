#pragma once

#include "MemoryObjects.h"

using namespace Urho3D;

#define ACTIVE_SHORTHEADEROBJECTCONTROL
#define ACTIVE_PACKEDOBJECTCONTROL
#define ACTIVE_OBJECTCOMMAND_BROADCASTING

enum GameNetworkMode
{
    AUTOMODE = 0,
    LOCALMODE,
    CLIENTMODE,
    SERVERMODE
};

enum NetCommand
{
    NOACTION = 0,
    ERASENODE,
    ENABLENODE,
    DISABLECLIENTOBJECTCONTROL,
    ADDNODE,
    EXPLODENODE,
    ADDFURNITURE,
    ADDCOLLECTABLE,
    TRANSFERITEM,
    DROPITEM,
    SETITEM,
    UPDATEEQUIPMENT,
    SETFULLEQUIPMENT,
    SETFULLINVENTORY,
    CHANGETILE,
    SETWEATHER,
    SETWORLD,
    TRIGCLICKED,
    ENTITYSELECTED,
    MOUNTENTITYON,
    UPDATEITEMSSTORAGE,
    MAX_NETCOMMAND
};

enum ObjectControlFlagBit
{
    OFB_ENABLED          = 0,
    OFB_NETSPAWNMODE     = 1,
    OFB_ANIMATIONCHANGED = 2,
};

enum
{
    OBJECTCONTROL = 1,
    OBJECTCOMMAND = 2
};

enum NetSpawnMode
{
    LOCALSPAWNONLY = 0,
    NETSPAWNONLY = 1
};

#ifdef ACTIVE_PACKEDOBJECTCONTROL
// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
float half_to_float(const unsigned short x);
unsigned short float_to_half(const float x);

struct PackedObjectControl
{
    // DataSize = 12 bytes + 20bytes + 14bytes = 46 bytes
    struct
    {
        unsigned short positionx_, positiony_;
        unsigned short velx_, vely_;
        unsigned short rotation_, direction_;
    } physics_; // 6short = 12bytes

    struct
    {
        unsigned int type_, spawnid_, buttons_;
        unsigned char entityid_, animstateindex_, animversion_;
        unsigned char viewZ_, flag_, stamp_;
        unsigned short totalDpsReceived_;
    } states_; // 3uint + 1short + 6bytes = 3*4 + 2 + 6 = 20bytes

    struct
    {
        unsigned int id_;
        unsigned short point1x_, point1y_;
        unsigned short point2x_, point2y_;
        unsigned short rot2_;
    } holderinfo_; // 1uint + 5short = 14 bytes
};
#endif

struct ObjectControl
{
    // DataSize = 24 bytes + 22bytes + 24bytes = 70 bytes

    struct
    {
        float positionx_, positiony_;
        float velx_, vely_;
        float rotation_, direction_;
    } physics_; // 6float = 24bytes

    struct
    {
        unsigned int type_, spawnid_, buttons_;
        unsigned char entityid_, animstateindex_, animversion_;
        unsigned char viewZ_, flag_, stamp_;
        float totalDpsReceived_;
    } states_; // 1float + 3uint + 6bytes = 4*4 + 6 = 22bytes

    struct
    {
        unsigned int id_;
        float point1x_, point1y_;
        float point2x_, point2y_;
        float rot2_;
    } holderinfo_; // 1uint + 5float = 24bytes

    void SetAnimationState(const StringHash& state);
    void SetFlagBit(int bit, bool state);

    bool IsEnabled() const { return (states_.flag_ & (1 << OFB_ENABLED)) != 0; }
    bool HasNetSpawnMode() const { return (states_.flag_ & (1 << OFB_NETSPAWNMODE)) != 0; }
    bool HasAnimationChanged() const { return (states_.flag_ & (1 << OFB_ANIMATIONCHANGED)) != 0; }
    StringHash GetAnimationState() const;

#ifdef ACTIVE_PACKEDOBJECTCONTROL
    void Pack(PackedObjectControl& packedobject);
    void Unpack(const PackedObjectControl& packedobject);
#endif
};

struct ObjectControlInfo
{
    ObjectControlInfo() : clientId_(0) { Clear(); }
    ObjectControlInfo(const ObjectControlInfo& c);

    void SetActive(bool enable, bool serverobject=true);
    void SetEnable(bool enable);

    void Clear();

    void BackupReceivedControl();
    void CopyReceivedControlTo(ObjectControl& ocontrol) const;
    void CopyReceivedControlAckTo(ObjectControl& ocontrol) const;
    void CopyPreparedControlTo(ObjectControl& ocontrol);
    void CopyPreparedToSendedControl();
    void CopyPreparedPositionToInitial();

    void UseInitialPosition(const ObjectControl& refcontrol);

    const ObjectControl& GetReceivedControl() const;
    const ObjectControl& GetPreviousReceivedControl() const;
    ObjectControl& GetReceivedControl();
    ObjectControl& GetPreparedControl();

    bool IsEnable() const { return GetReceivedControl().IsEnabled(); }

    void Read(VectorBuffer& msg);
    void ReadAck(VectorBuffer& msg);
    bool Write(VectorBuffer& msg);
    bool WriteAck(VectorBuffer& msg);

    void Dump() const;

    bool active_, prepared_;
    unsigned int clientId_;
    unsigned int serverNodeID_, clientNodeID_;
    unsigned int lastNetChangeCounter_;

#ifdef ACTIVE_PACKEDOBJECTCONTROL
    bool packed_;
    PackedObjectControl preparedPackedControl_;
    PackedObjectControl receivedPackedControl_;
#endif

    ObjectControl preparedControl_;
    ObjectControl receivedControls_[2];

    WeakPtr<Node> node_;
    List<ObjectControl> sendedControls_;

    static const ObjectControlInfo EMPTY;
};

class ObjectCommand : public PoolObject
{
public:
    ObjectCommand() { Clear(); }
    ObjectCommand(VectorBuffer& msg) { Read(msg); }
    ObjectCommand(const ObjectCommand& cmd) { cmd.CopyTo(*this); }

    void Read(VectorBuffer& msg);

    void Write(VectorBuffer& msg, int toclient=0) const;
    void CopyTo(ObjectCommand& cmd) const;
    void Dump() const;

    void Clear() { clientId_ = 0; broadCast_ = true; cmd_.Clear(); }
    void OnRestore() { Clear(); }

    int clientId_;
    bool broadCast_;
    VariantMap cmd_;

    static const ObjectCommand EMPTY;
};

enum ObjectCommandPacketState
{
    PACKET_CLEARED = 0,
    PACKET_PREPARED,
    PACKET_SENDED,
    PACKET_RECEIVED,
    PACKET_APPLIED,
};

class ObjectCommandPacket : public PoolObject
{
public:
    ObjectCommandPacket() { Clear(); }

    void Clear() { stamp_ = 0U; state_ = PACKET_CLEARED; buffer_.Clear(); }
    void OnRestore() { Clear(); }

    unsigned char stamp_;
    int state_;
    VectorBuffer buffer_;
};
