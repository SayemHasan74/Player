#pragma once
namespace CompactResizePolicy {
constexpr int height=160;
constexpr int collapseAt=200;
constexpr int expandAt=240;
constexpr bool collapse(bool mini,int proposedHeight){return !mini&&proposedHeight<=collapseAt;}
constexpr bool expand(bool mini,int proposedHeight){return mini&&proposedHeight>=expandAt;}
}
