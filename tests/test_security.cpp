#include <catch2/catch_test_macros.hpp>

#include <mshio/exception.h>
#include <mshio/mshio.h>

#include <cstdint>
#include <cstdlib>
#include <new>
#include <sstream>
#include <string>

namespace {

// While armed, any single allocation above the cap fails, so a parser that sizes containers
// from declared counts (instead of from data actually present) is caught deterministically.
constexpr size_t allocation_cap = size_t(16) << 20;
bool cap_armed = false;

void* allocate(size_t size)
{
    if (cap_armed && size > allocation_cap) throw std::bad_alloc();
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}

} // namespace

void* operator new(size_t size)
{
    return allocate(size);
}
void* operator new[](size_t size)
{
    return allocate(size);
}
void operator delete(void* p) noexcept
{
    std::free(p);
}
void operator delete[](void* p) noexcept
{
    std::free(p);
}
void operator delete(void* p, size_t) noexcept
{
    std::free(p);
}
void operator delete[](void* p, size_t) noexcept
{
    std::free(p);
}

namespace {

using namespace mshio;

// True when loading fails with a format error without any oversized allocation.
bool rejected_within_cap(const std::string& contents)
{
    std::istringstream in(contents);
    cap_armed = true;
    bool rejected = false;
    try {
        static_cast<void>(load_msh(in));
    } catch (const InvalidFormat&) {
        rejected = true;
    } catch (const std::bad_alloc&) {
    }
    cap_armed = false;
    return rejected;
}

std::string section(const char* version, const std::string& name, const std::string& payload)
{
    return std::string("$MeshFormat\n") + version + " 0 8\n$EndMeshFormat\n$" + name + "\n" +
           payload + "\n$End" + name + "\n";
}

template <typename T>
void binary(std::ostream& out, T value)
{
    out.write(reinterpret_cast<const char*>(&value), static_cast<std::streamsize>(sizeof(value)));
}

MshSpec triangle()
{
    MshSpec spec;
    spec.nodes.num_entity_blocks = 1;
    spec.nodes.num_nodes = 3;
    spec.nodes.min_node_tag = 1;
    spec.nodes.max_node_tag = 3;
    spec.nodes.entity_blocks = {{2, 1, 0, 3, {1, 2, 3}, {0, 0, 0, 1, 0, 0, 0, 1, 0}}};
    spec.elements.num_entity_blocks = 1;
    spec.elements.num_elements = 1;
    spec.elements.min_element_tag = 1;
    spec.elements.max_element_tag = 1;
    spec.elements.entity_blocks = {{2, 1, 2, 1, {1, 1, 2, 3}}};
    return spec;
}

} // namespace

TEST_CASE("Declared counts without data do not drive allocation", "[io][security]")
{
    CHECK(rejected_within_cap(section("4.1", "Nodes", "100000000 0 0 0")));
    CHECK(rejected_within_cap(section("4.1", "Nodes", "1 100000000 1 100000000\n3 1 0 100000000")));
    CHECK(rejected_within_cap(section("4.1", "Elements", "100000000 0 0 0")));
    CHECK(rejected_within_cap(section("4.1", "Elements", "1 100000000 1 1\n3 1 4 100000000")));
    CHECK(rejected_within_cap(section("4.1", "Entities", "100000000 0 0 0")));
    CHECK(rejected_within_cap(section("4.1", "Entities", "1 0 0 0\n1 0 0 0 100000000")));
    CHECK(rejected_within_cap(section("4.1", "PhysicalNames", "100000000")));
    CHECK(rejected_within_cap(section("4.1", "NodeData", "100000000")));
    CHECK(rejected_within_cap(section("4.1", "NodeData", "0\n0\n3\n0 1 100000000")));
    CHECK(rejected_within_cap(section("4.1", "ElementNodeData", "0\n0\n3\n0 2147483647 1\n1 1")));
    CHECK(rejected_within_cap(section("2.2", "Nodes", "2000000000")));
    CHECK(rejected_within_cap(section("2.2", "Elements", "1\n1 1 2000000000")));

    std::ostringstream out;
    out << "$MeshFormat\n4.1 1 8\n";
    binary(out, std::int32_t(1));
    out << "\n$EndMeshFormat\n$Nodes\n";
    for (std::uint64_t value : {UINT64_C(1), UINT64_C(100000000), UINT64_C(1), UINT64_C(100000000)})
        binary(out, value);
    for (std::int32_t value : {3, 1, 0}) binary(out, value);
    binary(out, UINT64_C(100000000));
    for (std::uint64_t tag = 1; tag <= 1000; ++tag) binary(out, tag);
    CHECK(rejected_within_cap(out.str()));
}

TEST_CASE("validate_spec rejects dangling and duplicate mesh tags", "[validate][security]")
{
    REQUIRE_NOTHROW(validate_spec(triangle()));

    SECTION("element references an absent node inside the tag range")
    {
        auto spec = triangle();
        spec.nodes.entity_blocks[0].tags = {1, 2, 4};
        spec.nodes.max_node_tag = 4;
        REQUIRE_THROWS_AS(validate_spec(spec), CorruptData);
    }
    SECTION("duplicate node tags")
    {
        auto spec = triangle();
        auto& block = spec.nodes.entity_blocks[0];
        block.tags = {1, 2, 3, 3};
        block.data.resize(12);
        block.num_nodes_in_block = 4;
        spec.nodes.num_nodes = 4;
        REQUIRE_THROWS_AS(validate_spec(spec), CorruptData);
    }
    SECTION("duplicate element tags")
    {
        auto spec = triangle();
        auto& block = spec.elements.entity_blocks[0];
        block.data = {1, 1, 2, 3, 1, 3, 2, 1};
        block.num_elements_in_block = 2;
        spec.elements.num_elements = 2;
        REQUIRE_THROWS_AS(validate_spec(spec), CorruptData);
    }
}
