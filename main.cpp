#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct ListNode {
    ListNode* prev = nullptr;
    ListNode* next = nullptr;
    ListNode* rand = nullptr;
    std::string data;
};

struct List {
    ListNode* head = nullptr;
    ListNode* tail = nullptr;
    std::size_t size = 0;
};

struct ParsedLine {
    std::string data;
    long long rand_index = -1;
};

namespace {

std::string trimAsciiWhitespace(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size()) {
        const char ch = value[begin];
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r' && ch != '\f' && ch != '\v') {
            break;
        }
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin) {
        const char ch = value[end - 1];
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r' && ch != '\f' && ch != '\v') {
            break;
        }
        --end;
    }

    return value.substr(begin, end - begin);
}

long long parseRandIndex(const std::string& text, std::size_t line_number) {
    const std::string cleaned = trimAsciiWhitespace(text);
    if (cleaned.empty()) {
        throw std::runtime_error("Line " + std::to_string(line_number) + " has an empty rand index");
    }

    std::size_t parsed = 0;
    long long value = 0;
    try {
        value = std::stoll(cleaned, &parsed, 10);
    } catch (const std::exception&) {
        throw std::runtime_error("Line " + std::to_string(line_number) + " has an invalid rand index: " + cleaned);
    }

    if (parsed != cleaned.size()) {
        throw std::runtime_error("Line " + std::to_string(line_number) + " has extra characters in rand index: " + cleaned);
    }

    return value;
}

void freeNodes(const std::vector<ListNode*>& nodes) {
    for (ListNode* node : nodes) {
        delete node;
    }
}

std::vector<ListNode*> collectNodes(const List& list) {
    if ((list.head == nullptr) != (list.tail == nullptr)) {
        throw std::runtime_error("List is structurally invalid: head/tail mismatch");
    }

    std::vector<ListNode*> nodes;
    nodes.reserve(list.size);

    ListNode* current = list.head;
    while (current != nullptr) {
        nodes.push_back(current);
        current = current->next;
    }

    if (nodes.size() != list.size) {
        throw std::runtime_error("List size metadata does not match the linked structure");
    }

    if (!nodes.empty() && nodes.back() != list.tail) {
        throw std::runtime_error("List tail does not match the linked structure");
    }

    return nodes;
}

void validateLinearLinks(const std::vector<ListNode*>& nodes, const std::string& list_name) {
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const ListNode* expected_prev = index == 0 ? nullptr : nodes[index - 1];
        const ListNode* expected_next = index + 1 < nodes.size() ? nodes[index + 1] : nullptr;

        if (nodes[index]->prev != expected_prev) {
            throw std::runtime_error(list_name + " has an invalid prev pointer at node " + std::to_string(index));
        }

        if (nodes[index]->next != expected_next) {
            throw std::runtime_error(list_name + " has an invalid next pointer at node " + std::to_string(index));
        }
    }
}

std::vector<ParsedLine> readTextInput(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Failed to open text input file: " + path);
    }

    std::vector<ParsedLine> result;
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const std::size_t separator = line.rfind(';');
        if (separator == std::string::npos) {
            throw std::runtime_error("Line " + std::to_string(line_number) + " does not contain ';'");
        }

        ParsedLine parsed;
        parsed.data = line.substr(0, separator);
        parsed.rand_index = parseRandIndex(line.substr(separator + 1), line_number);
        result.push_back(std::move(parsed));
    }

    return result;
}

List buildList(const std::vector<ParsedLine>& lines) {
    std::vector<ListNode*> nodes;
    nodes.reserve(lines.size());

    try {
        for (const ParsedLine& line : lines) {
            ListNode* node = new ListNode();
            node->data = line.data;
            nodes.push_back(node);
        }

        for (std::size_t index = 0; index < nodes.size(); ++index) {
            nodes[index]->prev = index == 0 ? nullptr : nodes[index - 1];
            nodes[index]->next = index + 1 < nodes.size() ? nodes[index + 1] : nullptr;
        }

        for (std::size_t index = 0; index < nodes.size(); ++index) {
            const long long rand_index = lines[index].rand_index;
            if (rand_index == -1) {
                nodes[index]->rand = nullptr;
                continue;
            }

            if (rand_index < 0 || rand_index >= static_cast<long long>(nodes.size())) {
                throw std::runtime_error(
                    "Invalid rand index at node " + std::to_string(index) + ": " + std::to_string(rand_index));
            }

            nodes[index]->rand = nodes[static_cast<std::size_t>(rand_index)];
        }
    } catch (...) {
        freeNodes(nodes);
        throw;
    }

    List list;
    if (!nodes.empty()) {
        list.head = nodes.front();
        list.tail = nodes.back();
    }
    list.size = nodes.size();
    return list;
}

void writeExact(std::ostream& output, const char* bytes, std::size_t count, const std::string& context) {
    if (count == 0) {
        return;
    }

    output.write(bytes, static_cast<std::streamsize>(count));
    if (!output) {
        throw std::runtime_error("Failed to write " + context);
    }
}

void readExact(std::istream& input, char* bytes, std::size_t count, const std::string& context) {
    if (count == 0) {
        return;
    }

    input.read(bytes, static_cast<std::streamsize>(count));
    if (!input) {
        throw std::runtime_error("Failed to read " + context);
    }
}

void writeUint64LE(std::ostream& output, std::uint64_t value, const std::string& context) {
    char buffer[sizeof(value)];
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        buffer[index] = static_cast<char>((value >> (index * 8U)) & 0xFFU);
    }
    writeExact(output, buffer, sizeof(buffer), context);
}

std::uint64_t readUint64LE(std::istream& input, const std::string& context) {
    char buffer[sizeof(std::uint64_t)];
    readExact(input, buffer, sizeof(buffer), context);

    std::uint64_t value = 0;
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        value |= static_cast<std::uint64_t>(static_cast<unsigned char>(buffer[index])) << (index * 8U);
    }
    return value;
}

void writeInt64LE(std::ostream& output, std::int64_t value, const std::string& context) {
    std::uint64_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    writeUint64LE(output, raw, context);
}

std::int64_t readInt64LE(std::istream& input, const std::string& context) {
    const std::uint64_t raw = readUint64LE(input, context);
    std::int64_t value = 0;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

void serialize(const List& list, const std::string& path) {
    const std::vector<ListNode*> nodes = collectNodes(list);
    validateLinearLinks(nodes, "Original list");

    std::unordered_map<const ListNode*, std::uint64_t> indices;
    indices.reserve(nodes.size());
    for (std::uint64_t index = 0; index < nodes.size(); ++index) {
        indices.emplace(nodes[static_cast<std::size_t>(index)], index);
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Failed to open binary output file: " + path);
    }

    writeUint64LE(output, nodes.size(), "node count");

    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const ListNode* node = nodes[index];
        const std::uint64_t data_length = static_cast<std::uint64_t>(node->data.size());
        writeUint64LE(output, data_length, "string length for node " + std::to_string(index));
        writeExact(output, node->data.data(), node->data.size(), "string bytes for node " + std::to_string(index));

        std::int64_t rand_index = -1;
        if (node->rand != nullptr) {
            const auto found = indices.find(node->rand);
            if (found == indices.end()) {
                throw std::runtime_error("Node " + std::to_string(index) + " has rand pointing outside the list");
            }
            rand_index = static_cast<std::int64_t>(found->second);
        }

        writeInt64LE(output, rand_index, "rand index for node " + std::to_string(index));
    }
}

List deserialize(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Failed to open binary input file: " + path);
    }

    const std::uint64_t node_count = readUint64LE(input, "node count");
    if (node_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("Binary file contains too many nodes to fit in memory");
    }

    std::vector<ListNode*> nodes;
    std::vector<std::int64_t> rand_indices;
    nodes.reserve(static_cast<std::size_t>(node_count));
    rand_indices.reserve(static_cast<std::size_t>(node_count));

    try {
        for (std::uint64_t index = 0; index < node_count; ++index) {
            const std::uint64_t data_length = readUint64LE(input, "string length for node " + std::to_string(index));
            if (data_length > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
                throw std::runtime_error("Node " + std::to_string(index) + " has a string too large to fit in memory");
            }

            std::string data(static_cast<std::size_t>(data_length), '\0');
            readExact(input, data.data(), data.size(), "string bytes for node " + std::to_string(index));

            ListNode* node = new ListNode();
            node->data = std::move(data);
            nodes.push_back(node);

            rand_indices.push_back(readInt64LE(input, "rand index for node " + std::to_string(index)));
        }

        for (std::size_t index = 0; index < nodes.size(); ++index) {
            nodes[index]->prev = index == 0 ? nullptr : nodes[index - 1];
            nodes[index]->next = index + 1 < nodes.size() ? nodes[index + 1] : nullptr;
        }

        for (std::size_t index = 0; index < nodes.size(); ++index) {
            const std::int64_t rand_index = rand_indices[index];
            if (rand_index == -1) {
                nodes[index]->rand = nullptr;
                continue;
            }

            if (rand_index < 0 || rand_index >= static_cast<std::int64_t>(nodes.size())) {
                throw std::runtime_error(
                    "Invalid rand index in binary file at node " + std::to_string(index) + ": " +
                    std::to_string(rand_index));
            }

            nodes[index]->rand = nodes[static_cast<std::size_t>(rand_index)];
        }
    } catch (...) {
        freeNodes(nodes);
        throw;
    }

    List list;
    if (!nodes.empty()) {
        list.head = nodes.front();
        list.tail = nodes.back();
    }
    list.size = nodes.size();
    return list;
}

void freeList(List& list) {
    ListNode* current = list.head;
    while (current != nullptr) {
        ListNode* next = current->next;
        delete current;
        current = next;
    }

    list.head = nullptr;
    list.tail = nullptr;
    list.size = 0;
}

std::vector<long long> randIndicesForComparison(const std::vector<ListNode*>& nodes) {
    std::unordered_map<const ListNode*, long long> indices;
    indices.reserve(nodes.size());
    for (long long index = 0; index < static_cast<long long>(nodes.size()); ++index) {
        indices.emplace(nodes[static_cast<std::size_t>(index)], index);
    }

    std::vector<long long> result;
    result.reserve(nodes.size());

    for (const ListNode* node : nodes) {
        if (node->rand == nullptr) {
            result.push_back(-1);
            continue;
        }

        const auto found = indices.find(node->rand);
        if (found == indices.end()) {
            throw std::runtime_error("A list contains rand pointing outside its node sequence");
        }
        result.push_back(found->second);
    }

    return result;
}

bool listsEqual(const List& left, const List& right) {
    const std::vector<ListNode*> left_nodes = collectNodes(left);
    const std::vector<ListNode*> right_nodes = collectNodes(right);

    if (left_nodes.size() != right_nodes.size()) {
        return false;
    }

    validateLinearLinks(left_nodes, "Original list");
    validateLinearLinks(right_nodes, "Restored list");

    const std::vector<long long> left_rand_indices = randIndicesForComparison(left_nodes);
    const std::vector<long long> right_rand_indices = randIndicesForComparison(right_nodes);

    for (std::size_t index = 0; index < left_nodes.size(); ++index) {
        if (left_nodes[index]->data != right_nodes[index]->data) {
            return false;
        }

        if (left_rand_indices[index] != right_rand_indices[index]) {
            return false;
        }
    }

    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        bool verify_round_trip = false;
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--verify") {
                verify_round_trip = true;
                continue;
            }

            throw std::runtime_error("Unknown argument: " + argument + ". Supported option: --verify");
        }

        const std::vector<ParsedLine> parsed_lines = readTextInput("inlet.in");

        List original = buildList(parsed_lines);
        try {
            serialize(original, "outlet.out");

            if (verify_round_trip) {
                List restored = deserialize("outlet.out");
                try {
                    if (!listsEqual(original, restored)) {
                        throw std::runtime_error("Deserialized list does not match the original list");
                    }
                } catch (...) {
                    freeList(restored);
                    throw;
                }
                freeList(restored);
            }
        } catch (...) {
            freeList(original);
            throw;
        }
        freeList(original);

        std::cout << "Serialized " << parsed_lines.size() << " nodes to outlet.out";
        if (verify_round_trip) {
            std::cout << " and verified deserialization successfully";
        }
        std::cout << ".\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
