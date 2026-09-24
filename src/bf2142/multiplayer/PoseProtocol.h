#pragma once
#include "../TrackingMath.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace bfvr::bf2142::net {
using Matrix=stereo::Matrix4;
using Secret=std::array<std::uint8_t,16>;
constexpr std::uint32_t Magic=0x31524e42;
constexpr std::uint16_t Version=2;
enum :std::uint16_t {Pose=1,Relay=2,Mirror=3};
enum :std::uint32_t {LeftValid=1,RightValid=2,WeaponHeld=4,LeftCrateHeld=8};
// Fixed-width, little-endian experimental PC protocol; no process pointers.
struct Packet {
 std::uint32_t magic=Magic;
 std::uint16_t version=Version,kind=Pose;
 std::uint32_t bytes=sizeof(Packet),sequence=0;
 std::uint64_t session=0;
 Secret secret{};
 std::uint32_t player=0,flags=0;
 Matrix body{},camera{},head{},left{},right{},weapon{};
 std::array<float,10> curls{};
 std::array<char,48> weaponName{};
 // Retransmitted event IDs are independent of pose sequence; consumed once.
 std::uint32_t snapSerial=0,throwSerial=0;
 float snapDegrees=0;stereo::Vec3 throwVelocity{};
 Matrix throwLaunch{};

};
static_assert(sizeof(Packet)==608);
static_assert(sizeof(Packet)<1200);
stereo::Vec3 TransformVelocity(stereo::Vec3,const Matrix&) noexcept;
struct EventWindow {
 std::uint64_t session=0,accepted=0;std::uint32_t serial=0;
 bool Accept(std::uint64_t sessionId,std::uint32_t event,std::uint64_t now,std::uint64_t minimumInterval=0) noexcept;
};
bool Validate(const Packet&,const Secret&) noexcept;
bool Decode(const void*,std::size_t,const Secret&,Packet*) noexcept;
bool Newer(std::uint32_t next,std::uint32_t previous) noexcept;
float Distance(const Matrix&,const Matrix&) noexcept;
struct FreshPose {
 Packet packet{};std::uint64_t received=0;
 bool Accept(const Packet&,std::uint64_t now) noexcept;
 const Packet* Read(std::uint64_t now,std::uint64_t lifetime=250) const noexcept;
 void Clear() noexcept {received=0;packet={};}
};
}
