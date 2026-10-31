#ifndef ULTIMATEMOP_RELOAD_REGISTRY_H
#define ULTIMATEMOP_RELOAD_REGISTRY_H

#include <cstddef>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace CustomReload
{
    struct Result
    {
        bool Success;
        std::size_t Rows;
        std::string Message;
    };

    class Registry
    {
    public:
        using Loader = std::function<Result()>;

        bool Register(std::string name, std::string description, Loader loader)
        {
            if (!ValidName(name) || !loader)
                return false;
            std::lock_guard<std::mutex> lock(_mutex);
            return _tables.emplace(std::move(name), std::make_shared<Entry>(std::move(description), std::move(loader))).second;
        }

        Result Reload(std::string const& name)
        {
            std::shared_ptr<Entry> entry;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto found = _tables.find(name);
                if (found == _tables.end())
                    return { false, 0, "Unknown custom table; use reload custom list" };
                entry = found->second;
            }
            // No catalog lock during I/O; reject recursive or overlapping reloads.
            std::unique_lock<std::recursive_mutex> lock(entry->Mutex, std::try_to_lock);
            if (!lock.owns_lock() || entry->Reloading)
                return { false, 0, "This table is already reloading" };
            entry->Reloading = true;
            struct ResetReloading
            {
                bool& Value;
                ~ResetReloading() { Value = false; }
            } reset{ entry->Reloading };
            try
            {
                return entry->Load();
            }
            catch (std::exception const& error)
            {
                return { false, 0, error.what() };
            }
            catch (...)
            {
                return { false, 0, "Loader threw a non-standard exception" };
            }
        }

        std::vector<std::pair<std::string, std::string>> List() const
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::vector<std::pair<std::string, std::string>> result;
            for (auto const& table : _tables)
                result.emplace_back(table.first, table.second->Description);
            return result;
        }

        static bool ValidName(std::string const& name)
        {
            if (name.empty() || name.size() > 64 || name[0] < 'a' || name[0] > 'z')
                return false;
            for (char character : name)
                if (!((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '_'))
                    return false;
            return name != "custom" && name != "list";
        }

    private:
        struct Entry
        {
            Entry(std::string description, Loader loader) : Description(std::move(description)), Load(std::move(loader)) { }
            std::string const Description;
            Loader const Load;
            std::recursive_mutex Mutex;
            bool Reloading = false;
        };
        mutable std::mutex _mutex;
        std::map<std::string, std::shared_ptr<Entry>> _tables;
    };

    inline Registry& Tables()
    {
        static Registry registry;
        return registry;
    }
}

#endif
