/* C API for the paper experiments: one-pair 3GPP UMa NLOS spatial channel.

   ns3_channel_gains_scm draws cluster delays, angles, and small-scale
   fading from TR 38.901. 
*/

#include "ns3/antenna-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/propagation-module.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/uniform-planar-array.h"

#include <cmath>
#include <complex>

using namespace ns3;

namespace
{

const double H_BS = 25.0;
const double H_UE = 1.5;

struct NodeMob
{
    Ptr<Node> node;
    Ptr<ConstantPositionMobilityModel> mob;
};

Ptr<ThreeGppUmaPropagationLossModel> g_scmPl;
NodeMob g_scmBs;
NodeMob g_scmUe;
Ptr<UniformPlanarArray> g_scmBsAnt;
Ptr<UniformPlanarArray> g_scmUeAnt;
bool g_scmPair = false;
bool g_rng_ok = false;

NodeMob
MakeNode()
{
    NodeMob nm;
    nm.node = CreateObject<Node>();
    nm.mob = CreateObject<ConstantPositionMobilityModel>();
    nm.node->AggregateObject(nm.mob);
    return nm;
}

void
EnsureScmPair()
{
    if (g_scmPair)
    {
        return;
    }
    if (!g_rng_ok)
    {
        RngSeedManager::SetSeed(1);
        RngSeedManager::SetRun(1);
        g_rng_ok = true;
    }
    g_scmPl = CreateObject<ThreeGppUmaPropagationLossModel>();
    g_scmPl->SetAttribute("Frequency", DoubleValue(2.0e9));
    g_scmPl->SetAttribute("ShadowingEnabled", BooleanValue(false));
    g_scmPl->SetChannelConditionModel(CreateObject<NeverLosChannelConditionModel>());
    g_scmBs = MakeNode();
    g_scmUe = MakeNode();
    g_scmBsAnt = CreateObject<UniformPlanarArray>();
    g_scmBsAnt->SetAttribute("NumRows", UintegerValue(1));
    g_scmBsAnt->SetAttribute("NumColumns", UintegerValue(1));
    g_scmBsAnt->SetAttribute("AntennaElement", PointerValue(CreateObject<ThreeGppAntennaModel>()));
    g_scmUeAnt = CreateObject<UniformPlanarArray>();
    g_scmUeAnt->SetAttribute("NumRows", UintegerValue(1));
    g_scmUeAnt->SetAttribute("NumColumns", UintegerValue(1));
    g_scmUeAnt->SetAttribute("AntennaElement", PointerValue(CreateObject<IsotropicAntennaModel>()));
    g_scmPair = true;
}

Ptr<ThreeGppChannelModel>
MakeScm(int64_t stream)
{
    Ptr<ThreeGppChannelModel> scm = CreateObject<ThreeGppChannelModel>();
    scm->SetAttribute("Frequency", DoubleValue(2.0e9));
    scm->SetAttribute("Scenario", StringValue("UMa"));
    scm->SetAttribute("UpdatePeriod", TimeValue(MilliSeconds(0)));
    scm->SetChannelConditionModel(CreateObject<NeverLosChannelConditionModel>());
    scm->AssignStreams(stream);
    return scm;
}

double
FadePower(Ptr<const MatrixBasedChannelModel::ChannelMatrix> mat)
{
    double p = 0.0;
    for (const auto& u : mat->m_channel)
    {
        for (const auto& s : u)
        {
            for (const auto& c : s)
            {
                p += std::norm(c);
            }
        }
    }
    return p;
}

} // namespace

extern "C" int
ns3_channel_gains_scm(int nUsers, int nAnts, const double* ux, const double* uy,
                      const double* ax, const double* ay, const double* tilt,
                      const double* az, double* H, int64_t stream)
{
    if (nUsers <= 0 || nAnts <= 0 || ux == nullptr || uy == nullptr || ax == nullptr
        || ay == nullptr || tilt == nullptr || az == nullptr || H == nullptr)
    {
        return 1;
    }
    EnsureScmPair();
    int64_t idx = 0;
    for (int u = 0; u < nUsers; ++u)
    {
        Vector ue(ux[u], uy[u], H_UE);
        g_scmUe.mob->SetPosition(ue);
        for (int i = 0; i < nAnts; ++i)
        {
            Vector bs(ax[i], ay[i], H_BS);
            g_scmBs.mob->SetPosition(bs);
            g_scmBsAnt->SetAttribute("BearingAngle", DoubleValue(DegreesToRadians(az[i])));
            g_scmBsAnt->SetAttribute("DowntiltAngle", DoubleValue(DegreesToRadians(tilt[i])));
            Ptr<ThreeGppChannelModel> scm = MakeScm(stream + idx);
            double rx0 = g_scmPl->CalcRxPower(0.0, g_scmBs.mob, g_scmUe.mob);
            auto mat = scm->GetChannel(g_scmBs.mob, g_scmUe.mob, g_scmBsAnt, g_scmUeAnt);
            H[u * nAnts + i] = std::pow(10.0, rx0 / 10.0) * FadePower(mat);
            idx += 1;
        }
    }
    return 0;
}
