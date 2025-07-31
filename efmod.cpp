#include "efmod-api/efmod_core.hpp"
#include "tefmod-api/BaseType.hpp"
#include "tefmod-api/Logger.hpp"
#include "tefmod-api/TEFMod.hpp"

TEFMod::Logger* g_log;
TEFMod::TEFModAPI* g_api;

struct ItemFields {
    TEFMod::Field<int>* maxStack;
    TEFMod::Field<bool>* consumable;
} g_item;

void (*original_SetDefaults)(TEFMod::TerrariaInstance, int, bool, TEFMod::TerrariaInstance);
void SetDefaults_T(TEFMod::TerrariaInstance, int, bool, TEFMod::TerrariaInstance);
inline TEFMod::HookTemplate HookTemplate_SetDefaults {
        reinterpret_cast<void*>(SetDefaults_T),
        {  }
};

void SetDefaults_T(TEFMod::TerrariaInstance i, int t, bool n, TEFMod::TerrariaInstance v) {
    original_SetDefaults(i, t, n, v);
    for (const auto fun: HookTemplate_SetDefaults.FunctionArray) {
        if(fun) reinterpret_cast<void(*)(TEFMod::TerrariaInstance, int, bool, TEFMod::TerrariaInstance)>(fun)(i, t, n, v);
    }
}

void Hook_SetDefaults(TEFMod::TerrariaInstance item, int type, bool noMatCheck, TEFMod::TerrariaInstance variant) {
    const bool should = (
            type == 560||
            type == 43||
            type == 70||
            type == 1331||
            type ==1133||
            type ==5120||
            type ==4988||
            type ==544||
            type ==556||
            type ==557||
            type ==1293||
            type ==3601||
            type ==5334||
            type ==1958||
            type ==1844||
            type ==3828||
            type ==1315
    );

    if (should) {
        g_log->d("修正 id=", type);
        g_item.maxStack->Set(1, item);
        g_item.consumable->Set(false,item);
    }
}

class NonConsumable final : public EFMod {
public:

    int Initialize(const std::string &path, MultiChannel *multiChannel) override {return 0;}
    int UnLoad(const std::string &path, MultiChannel *multiChannel) override {return 0;}

    int Load(const std::string &path, MultiChannel* channel) override {
        g_api = channel->receive<TEFMod::TEFModAPI*>("TEFMod::TEFModAPI");
        g_log = channel->receive<TEFMod::Logger*(*)(const std::string& Tag, const std::string& filePath, const std::size_t)>("TEFMod::CreateLogger")("NonConsumable-lzup", "", 0);

        g_log->init();

        return 0;
    }

    void Send(const std::string &path, MultiChannel* channel) override {
        g_api->registerFunctionDescriptor({
            "Terraria", "Item", "SetDefaults", "hook>>void",
            3,
            &HookTemplate_SetDefaults,
            { (void*)Hook_SetDefaults }
        });

        const char* fields[] = {"maxStack","consumable"};

        for (auto& name : fields) {
            g_api->registerApiDescriptor({"Terraria", "Item", name, "Field"});
        }
    }

    void Receive(const std::string &path, MultiChannel* channel) override {
        const auto ParseIntField = channel->receive<TEFMod::Field<int>*(*)(void*)>(
                "TEFMod::Field<Int>::ParseFromPointer");
        const auto ParseBoolField = channel->receive<TEFMod::Field<bool>*(*)(void*)>(
                "TEFMod::Field<Int>::ParseFromPointer");

        // 初始化字段
        g_item.maxStack = ParseIntField(g_api->GetAPI<void*>(
                {"Terraria", "Item", "maxStack", "Field"}));
        g_item.consumable = ParseBoolField(g_api->GetAPI<void*>(
                {"Terraria", "Item", "consumable", "Field"}));
        original_SetDefaults = g_api->GetAPI<void(*)(TEFMod::TerrariaInstance, int, bool, TEFMod::TerrariaInstance)>(
                {"Terraria", "Item", "SetDefaults", "old_fun", 3});
    }

    Metadata GetMetadata() override {
        return {
                "NonConsumable",
                "lzup",
                "1.1.1",
                20250517,
                ModuleType::Game,
                { false }
        };
    }
};

EFMod* CreateMod() {
    static NonConsumable instance;
    return &instance;
}