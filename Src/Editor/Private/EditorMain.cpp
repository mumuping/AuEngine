#include "Application/Application.h"

#include "Core/Log.h"

namespace Au
{
    namespace
    {
        class EditorApplication final : public Application
        {
        public:
            EditorApplication()
                : Application({.Name = "AuEditor"})
            {
            }

        protected:
            bool OnInitialize() override
            {
                AU_LOG_INFO("Editor initialized");
                return true;
            }
        };
    } // namespace
} // namespace Au

int main(int argc, char** argv)
{
    Au::EditorApplication app;
    return app.Run(argc, argv);
}
