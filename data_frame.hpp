/**
 * @file data_frame.hpp
 * @author okano tomoyuki (tomoyuki.okano@tsuneishi.com)
 * @brief 表形式データを取り扱う @ref Utility::DataFrame クラスの定義ヘッダー
 * @version 0.1
 * @date 2024-01-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef UTILITY_DATA_FRAME_HPP
#define UTILITY_DATA_FRAME_HPP

#include <vector>               // std::vector
#include <cstdlib>              // std::strtol, std::strtod
#include <string>               // std::string, std::getline
#include <fstream>              // std::ifstream, std::ofstream
#include <sstream>              // std::sstream
#include <stdexcept>            // std::runtime_error, std::out_of_range
#include <iostream>             // std::cout, std::endl
#include <algorithm>            // std::find
#include <unordered_map>        // std::unordered_map
#include <memory>               // std::shared_ptr
#include <tuple>                // std:::tuple
#include <numeric>              // std::iota
#include <functional>           // std::function

class DataFrame final
{

public:

    void operator=(const DataFrame& other)
    {
        rows_    = other.rows_;
        cols_    = other.cols_;
        header_  = other.header_;
        data_    = other.data_;
    }

    static DataFrame read_csv(const std::string& file_path, const bool& read_header=true, const std::string& separator = ",", const std::string& new_line = "\r\n", const bool& auto_trim = true)
    {
        auto header     = std::make_shared<std::vector<std::string>>();
        auto data       = std::make_shared<std::vector<std::vector<std::string>>>();

        std::ifstream ifs(file_path, std::ios_base::binary);
        if(!ifs)
            throw std::runtime_error("file '" + file_path + "' doesn't exist.");

        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string buffer = ss.str();
        auto line_list = split(buffer, new_line);
        while(line_list.back().empty())
            line_list.pop_back();

        // switching header on/off.
        if(read_header)
        {
            *header = split(line_list.front(), separator, auto_trim);
            line_list.erase(line_list.begin());
        }
        else
        {
            for(auto i=0; i < split(line_list.front(), separator, auto_trim).size();i++) 
                header->push_back(std::to_string(i));
        }

        int row_index = 0;
        data->reserve(line_list.size());
        for(const auto& line : line_list)
        {
            const auto row = split(line, separator, auto_trim);
            if(row.size() != header->size())
            {
                std::stringstream ss;
                ss  << "line[" << row_index << "] element size between header and row is different."
                    << "header's element size : " << header->size() << "row's element size : " << row.size();  
                throw std::runtime_error(ss.str());
            }
            data->push_back(row);
            row_index++;
        }

        auto rows = std::vector<size_t>(line_list.size());
        std::iota(rows.begin(), rows.end(), 0);

        auto cols = std::vector<size_t>(header->size());
        std::iota(cols.begin(), cols.end(), 0);

        return DataFrame{rows, cols, header, data};
    }

    DataFrame& filter(const std::function<bool(const DataFrame& row)>& func)
    {
        auto rows = std::vector<size_t>();
        for (const auto& row : rows_)
        {
            if (func(DataFrame{std::vector<size_t>{row}, cols_, header_, data_}))
            {
                rows.push_back(row);
            }
        }
        rows_ = rows;
        return *this;
    }

    DataFrame& sort_by(const char* col_name, const bool& ascending = true)
    {
        const auto it = std::find_if(cols_.begin(), cols_.end(), [&](const std::size_t& col) { return col_name == (*header_)[col]; });
        if (it == cols_.end())
            throw std::out_of_range("column name '" + std::string(col_name) + "' is not found.");       
    
        std::sort(rows_.begin(), rows_.end(), [&](const std::size_t& lhs, const std::size_t& rhs) {
            return (ascending ? (*data_)[lhs][*it] < (*data_)[rhs][*it] : (*data_)[lhs][*it] > (*data_)[rhs][*it]);
        });

        return *this;
    }

    bool to_csv(const std::string& file_path, const std::string& separator = ",", const std::string& new_line = "\r\n") const
    {
        auto ofs    = std::ofstream(file_path, std::ios_base::binary);
        
        if(!ofs)    
            return false;

        auto line = std::string();
        for (const auto& col : cols_)
        {
            line += header_->operator[](col) + separator;
        }
        ofs << line.substr(0, line.size() - separator.size());

        for (const auto& row : rows_)
        {
            line = new_line;
            for (const auto& col : cols_)
            {
                line += (*data_)[row][col] + separator;
            }
            ofs << line.substr(0, line.size() - separator.size());
        }

        return true;
    }

    std::vector<DataFrame> rows()
    {
        auto ret = std::vector<DataFrame>();
        for (const auto& row : rows_)
        {
            ret.push_back(DataFrame{std::vector<std::size_t>{row}, cols_, header_, data_});
        }
        return ret;
    }

    std::vector<DataFrame> reverse_rows()
    {
        auto ret    = std::vector<DataFrame>();
        auto rows   = rows_;
        std::reverse(rows.begin(), rows.end());
        for (const auto& row : rows)
        {
            ret.push_back(DataFrame{std::vector<std::size_t>{row}, cols_, header_, data_});
        }
        return ret;
    }

    std::vector<DataFrame> cols()
    {
        auto ret = std::vector<DataFrame>();
        for (const auto& col : cols_)
        {
            ret.push_back(DataFrame{rows_, std::vector<std::size_t>{col}, header_, data_ });
        }
        return ret;
    }

    std::vector<DataFrame> reverse_cols()
    {
        auto ret    = std::vector<DataFrame>();
        auto cols   = cols_;
        std::reverse(cols.begin(), cols.end());
        for (const auto& col : cols)
        {
            ret.push_back(DataFrame{rows_, std::vector<std::size_t>{col}, header_, data_ });
        }
        return ret;
    }

    std::size_t row_size() const
    {
        return rows_.size();
    }

    std::size_t col_size() const
    {
        return cols_.size();
    }

    std::vector<std::string> header() const
    {
        auto ret = std::vector<std::string>();
        for (const auto& col : cols_)
        {
            ret.push_back(header_->operator[](col));
        }
        return ret;
    }

    DataFrame copy() const
    {
        auto header = std::make_shared<std::vector<std::string>>();
        auto data   = std::make_shared<std::vector<std::vector<std::string>>>();
        *header     = *header_;
        *data       = *data_;
        return DataFrame{rows_, cols_, header, data};
    }

    DataFrame operator[](const char* col_name) const
    {
        auto it = std::find_if(cols_.begin(), cols_.end(), [&](const std::size_t& col) { return col_name == (*header_)[col]; });
        if (it == cols_.end())
            throw std::out_of_range("column name '" + std::string(col_name) + "' is not found.");
        return DataFrame{rows_, std::vector<size_t>{*it}, header_, data_};
    }

    DataFrame operator[](const std::vector<const char*>& col_names) const
    {
        auto cols = std::vector<size_t>();
        for (const  auto& col_name : col_names)
        {
            auto it = std::find_if(cols_.begin(), cols_.end(), [&](const std::size_t& col) { return col_name == (*header_)[col]; });
            if (it == cols_.end())
                throw std::out_of_range("column name '" + std::string(col_name) + "' is not found.");
            cols.push_back(std::distance(cols_.begin(), it));
        }
        return DataFrame{rows_, cols, header_, data_};
    }

    DataFrame operator[](const int& row_index) const
    {
        const auto index = (row_index >= 0) ? row_index : rows_.size() + row_index;
        if (index < 0 || index >= rows_.size())
            throw std::out_of_range("row index number was out of range");
        return DataFrame{std::vector<size_t>{rows_[index]}, cols_, header_, data_};
    }

    DataFrame operator[](const std::vector<int>& row_indices) const
    {
        auto rows = std::vector<size_t>();
        for (const auto& row_index : row_indices)
        {
            const auto index = (row_index >= 0) ? row_index : rows_.size() + row_index;
            if (index < 0 || index >= rows_.size())
                throw std::out_of_range("row index number was out of range");
            rows.push_back(rows_[index]);
        }
        return DataFrame{rows, cols_, header_, data_};
    }

    DataFrame operator[](const std::tuple<const char*, int>& access) const
    {
        return (*this)[std::get<0>(access)][std::get<1>(access)];
    }

    DataFrame operator[](const std::tuple<std::vector<const char*>, std::vector<int>>& access) const
    {
        return (*this)[std::get<0>(access)][std::get<1>(access)];
    }

    DataFrame& rename_header(const char* from, const char* to)
    {
        auto it = std::find_if(cols_.begin(), cols_.end(), [&](const std::size_t& col) { return from == header_->operator[](col); });
        if (it == cols_.end())
            throw std::out_of_range("column name '" + std::string(from) + "' is not found.");
        header_->operator[](*it) = to;
        return *this;
    }

    DataFrame& rename_header(const std::vector<const char*>& header)
    {
        if (header.size() != cols_.size())
            throw std::runtime_error("header size is different");

        for (size_t i = 0; i < header.size(); i++)
        {
            header_->operator[](cols_[i]) = header[i];
        }

        return *this;
    }

    DataFrame& rename_header(const std::vector<std::tuple<const char*, const char*>>& header)
    {
        for (const auto& key_value : header)
        {
            rename_header(std::get<0>(key_value), std::get<1>(key_value));
        }
        return *this;
    }

    void describe() const
    {
        std::cout << "row size : " << rows_.size() << std::endl;
        std::cout << "col size : " << cols_.size() << " [ ";
        for (const auto& col : cols_)
        {
            std::cout << "\"" << (*header_)[col] << "\" ";
        }
        std::cout << "]" << std::endl;
    }

    template<typename T>
    inline T as() const
    {
        auto ret = T();
        as_impl(ret);
        return ret;
    }

    template<typename T>
    inline void operator=(const T& other)
    {
        assign(other);
    }

    friend std::ostream& operator<<(std::ostream& os, const DataFrame& df)
    {
        auto widths = std::vector<size_t>();
        for (const auto& col : df.cols_)
        {
            auto width = (*df.header_)[col].size();
            for (const auto& row : df.rows_)
            {
                width = (width > (*df.data_)[row][col].size()) ? width : (*df.data_)[row][col].size();
            }
            widths.push_back(width);
        }

        auto line = std::string();
        for (auto i= 0; i < df.cols_.size(); i++)
        {
            line += "| " + (*df.header_)[df.cols_[i]] + std::string(widths[i] - (*df.header_)[df.cols_[i]].size(), ' ') + " ";
        }
        line += "|";
        os << line << std::endl;
        os << std::string(line.size(), '-');

        for (const auto& row : df.rows_)
        {
            std::cout << std::endl;
            line.clear();
            for (auto i= 0; i < df.cols_.size(); i++)
            {
                line += "| " + (*df.data_)[row][df.cols_[i]] + std::string(widths[i] - (*df.data_)[row][df.cols_[i]].size(), ' ') + " ";
            }
            line += "|";
            os << line;
        }

        return os;
    }

private:

    std::vector<std::size_t>     rows_;
    std::vector<std::size_t>     cols_;

    std::shared_ptr<std::vector<std::string>>   header_;
    std::shared_ptr<std::vector<std::vector<std::string>>>   data_;

    DataFrame(
        const std::vector<size_t>& rows,
        const std::vector<size_t>& cols,
        const std::shared_ptr<std::vector<std::string>>& header, 
        const std::shared_ptr<std::vector<std::vector<std::string>>>& data
    )   : rows_(rows)
        , cols_(cols)
        , header_(header)
        , data_(data)
    {}

    void as_impl(bool& ret) const
    {
        if (rows_.size() != 1 || cols_.size() != 1)
            throw std::runtime_error("as() failed. data size is not 1");

        const auto& str = (*data_)[rows_[0]][cols_[0]];
        
        if (str == "true" || str == "True" || str == "TRUE" || str == "1")
            ret = true;
        else if (str == "false" || str == "False" || str == "FALSE" || str == "0")
            ret = false;
        else
            throw std::runtime_error("as() failed. '" + str + "' could not cast to bool");
    }

    void as_impl(int& ret) const
    {
        if (rows_.size() != 1 || cols_.size() != 1)
            throw std::runtime_error("as() failed. data size is not 1");

        const auto& str = (*data_)[rows_[0]][cols_[0]];
        char* endptr    = nullptr;
        ret             = std::strtol(str.c_str(), &endptr, 10);

        if (endptr == str.c_str())
            throw std::runtime_error("as() failed. '" + str + "' could not cast to int");
    }

    void as_impl(double& ret) const
    {
        if (rows_.size() != 1 || cols_.size() != 1)
            throw std::runtime_error("as() failed. data size is not 1");

        const auto& str = (*data_)[rows_[0]][cols_[0]];
        char* endptr    = nullptr;
        ret             = std::strtod(str.c_str(), &endptr);
        if (endptr == str.c_str())
            throw std::runtime_error("as() failed. '" + str + "' could not cast to double");
    }

    void as_impl(std::string& ret) const
    {
        if (rows_.size() != 1 || cols_.size() != 1)
            throw std::runtime_error("as() failed. data size is not 1");

        ret = (*data_)[rows_[0]][cols_[0]];
    }

    void as_impl(std::vector<bool>& ret) const
    {
        if (!(rows_.size() == 1 || cols_.size() == 1))
            throw std::runtime_error("as() failed. data size is not 1");

        if (rows_.size() == 1)
        {
            for (const auto& col : cols_)
            {
                const auto& str = (*data_)[rows_[0]][col];
                if (str == "true" || str == "True" || str == "TRUE" || str == "1")
                    ret.push_back(true);
                else if (str == "false" || str == "False" || str == "FALSE" || str == "0")
                    ret.push_back(false);
                else
                    throw std::runtime_error("as() failed. '" + str + "' could not cast to bool");
            }
        }
        else if (cols_.size() == 1)
        {
            for (const auto& row : rows_)
            {
                const auto& str = (*data_)[row][cols_[0]];
                if (str == "true" || str == "True" || str == "TRUE" || str == "1")
                    ret.push_back(true);
                else if (str == "false" || str == "False" || str == "FALSE" || str == "0")
                    ret.push_back(false);
                else
                    throw std::runtime_error("as() failed. '" + str + "' could not cast to bool");
            }
        }
    }

    void as_impl(std::vector<int>& ret) const
    {
        if (!(rows_.size() == 1 || cols_.size() == 1))
            throw std::runtime_error("as() failed. data size is not 1");

        if (rows_.size() == 1)
        {
            for (const auto& col : cols_)
            {
                const auto& str = (*data_)[rows_[0]][col];
                char* endptr    = nullptr;
                int r           = std::strtol(str.c_str(), &endptr, 10);
                if (endptr == str.c_str())
                    throw std::runtime_error("as() failed. '" + str + "' could not cast to int");
                ret.push_back(r);
            }
        }
        else if (cols_.size() == 1)
        {
            for (const auto& row : rows_)
            {
                const auto& str = (*data_)[row][cols_[0]];
                char* endptr    = nullptr;
                int r           = std::strtol(str.c_str(), &endptr, 10);
                if (endptr == str.c_str())
                    throw std::runtime_error("as() failed. '" + str + "' could not cast to int");
                ret.push_back(r);
            }
        }
    }

    void as_impl(std::vector<double>& ret) const
    {
        if (!(rows_.size() == 1 || cols_.size() == 1))
            throw std::runtime_error("as() failed. data size is not 1");

        if (rows_.size() == 1)
        {
            for (const auto& col : cols_)
            {
                const auto& str = (*data_)[rows_[0]][col];
                char* endptr    = nullptr;
                double r        = std::strtod(str.c_str(), &endptr);
                if (endptr == str.c_str())
                    throw std::runtime_error("as() failed. '" + str + "' could not cast to double");
                ret.push_back(r);
            }
            return;
        }
        else if (cols_.size() == 1)
        {
            for (const auto& row : rows_)
            {
                const auto& str = (*data_)[row][cols_[0]];
                char* endptr    = nullptr;
                double r        = std::strtod(str.c_str(), &endptr);
                if (endptr == str.c_str())
                    throw std::runtime_error("as() failed. '" + str + "' could not cast to double");
                ret.push_back(r);
            }
            return;
        }
    }

    void as_impl(std::vector<std::string>& ret) const
    {
        if (!(rows_.size() == 1 || cols_.size() == 1))
            throw std::runtime_error("as() failed. data size is not 1");

        if (rows_.size() == 1)
        {
            for (const auto& col : cols_)
            {
                ret.push_back((*data_)[rows_[0]][col]);
            }
        }
        else if (cols_.size() == 1)
        {
            for (const auto& row : rows_)
            {
                ret.push_back((*data_)[row][cols_[0]]);
            }
        }
    }

    void as_impl(std::vector<std::vector<bool>>& ret) const
    {
        for (const auto& row : rows_)
        {
            ret.push_back(std::vector<bool>{});
            for (const auto& col : cols_)
            {
                const auto& str = (*data_)[row][col];
                if (str == "true" || str == "True" || str == "TRUE" || str == "1")
                    ret.back().push_back(true);
                else if (str == "false" || str == "False" || str == "FALSE" || str == "0")
                    ret.back().push_back(false);
                else
                    throw std::runtime_error("as() failed. '" + str + "' could not cast to bool");
            }
        }
    }

    void as_impl(std::vector<std::vector<int>>& ret) const
    {
        for (const auto& row : rows_)
        {
            ret.push_back(std::vector<int>{});
            for (const auto& col : cols_)
            {
                const auto& str = (*data_)[row][col];
                char* endptr    = nullptr;
                int r           = std::strtol(str.c_str(), &endptr, 10);
                if (endptr == str.c_str())
                    throw std::runtime_error("as() failed. '" + str + "' could not cast to int");
                ret.back().push_back(r);
            }
        }
    }

    void as_impl(std::vector<std::vector<std::string>>& ret) const
    {
        for (const auto& row : rows_)
        {
            ret.push_back(std::vector<std::string>{});
            for (const auto& col : cols_)
            {
                ret.back().push_back((*data_)[row][col]);
            }
        }
    }

    void assign(const bool& value)
    {
        if (rows_.size() != 1 && cols_.size() != 1)
            throw std::runtime_error("operator=() failed. data size is not 1");

        (*data_)[rows_[0]][cols_[0]] = value ? "true" : "false";
    }

    void assign(const int& value)
    {
        if (rows_.size() != 1 && cols_.size() != 1)
            throw std::runtime_error("operator=() failed. data size is not 1");

        (*data_)[rows_[0]][cols_[0]] = std::to_string(value);
    }

    void assign(const double& value)
    {
        if (rows_.size() != 1 && cols_.size() != 1)
            throw std::runtime_error("operator=() failed. data size is not 1");

        (*data_)[rows_[0]][cols_[0]] = std::to_string(value);
    }

    void assign(const char* value)
    {
        if (rows_.size() != 1 && cols_.size() != 1)
            throw std::runtime_error("operator=() failed. data size is not 1");

        (*data_)[rows_[0]][cols_[0]] = value;
    }

    void assign(const std::string& value)
    {
        if (rows_.size() != 1 && cols_.size() != 1)
            throw std::runtime_error("operator=() failed. data size is not 1");
        
        (*data_)[rows_[0]][cols_[0]] = value;
    }

    void assign(const std::vector<bool>& value)
    {
        if (!((rows_.size() == 1 && cols_.size() == value.size()) || (rows_.size() == value.size() && cols_.size() == 1)))
            throw std::runtime_error("operator=() failed. data size is not 1");

        if (rows_.size() == 1 && cols_.size() == value.size())
        {
            for (std::size_t i = 0; i < value.size(); ++i)
            {
                (*data_)[rows_[0]][cols_[i]] = value[i] ? "true" : "false";
            }
        }
        else
        {
            for (std::size_t i = 0; i < value.size(); ++i)
            {
                (*data_)[rows_[i]][cols_[0]] = value[i] ? "true" : "false";
            }
        }
    }

    void assign(const std::vector<int>& value)
    {
        if (!((rows_.size() == 1 && cols_.size() == value.size()) || (rows_.size() == value.size() && cols_.size() == 1)))
            throw std::runtime_error("operator=() failed. data size is not 1");
        
        if (rows_.size() == 1 && cols_.size() == value.size())
        {
            for (auto i = 0; i < value.size(); ++i)
            {
                (*data_)[rows_[0]][cols_[i]] = std::to_string(value[i]);
            }
        }
        else
        {
            for (auto i = 0; i < value.size(); ++i)
            {
                (*data_)[rows_[i]][cols_[0]] = std::to_string(value[i]);
            }
        }
    }
    
    void assign(const std::vector<double>& value)
    {
        if (!((rows_.size() == 1 && cols_.size() == value.size()) || (rows_.size() == value.size() && cols_.size() == 1)))
            throw std::runtime_error("operator=() failed. data size is not 1");

        if (rows_.size() == 1 && cols_.size() == value.size())
        {
            for (auto i = 0; i < value.size(); ++i)
            {
                (*data_)[rows_[0]][cols_[i]] = std::to_string(value[i]);
            }
        }
        else
        {
            for (auto i = 0; i < value.size(); ++i)
            {
                (*data_)[rows_[i]][cols_[0]] = std::to_string(value[i]);
            }
        }
    }

    void assign(const std::vector<std::vector<bool>>& value)
    {
        if (value.size() != rows_.size())
            throw std::runtime_error("operator=() failed. data size not match");
        
        for (auto i = 0; i < value.size(); i++)
        {
            if (value[i].size() != cols_.size())
                throw std::runtime_error("operator=() failed. data size not match");
        }

        for (auto i = 0; i < value.size(); ++i)
        {
            for (auto j = 0; j < value[i].size(); ++j)
            {
                (*data_)[rows_[i]][cols_[j]] = value[i][j] ? "true" : "false";
            }
        }
    }

    void assign(const std::vector<std::vector<int>>& value)
    {
        if (value.size() != rows_.size())
            throw std::runtime_error("operator=() failed. data size not match");

        for (auto i = 0; i < value.size(); i++)
        {
            if (value[i].size() != cols_.size())
                throw std::runtime_error("operator=() failed. data size not match");
        }

        for (auto i = 0; i < value.size(); ++i)
        {
            for (auto j = 0; j < value[i].size(); ++j)
            {
                (*data_)[rows_[i]][cols_[j]] = std::to_string(value[i][j]);
            }
        }
    }

    void assign(const std::vector<std::vector<double>>& value)
    {
        if (value.size() != rows_.size())
            throw std::runtime_error("operator=() failed. data size not match");

        for (auto i = 0; i < value.size(); i++)
        {
            if (value[i].size() != cols_.size())
                throw std::runtime_error("operator=() failed. data size not match");
        }

        for (auto i = 0; i < value.size(); ++i)
        {
            for (auto j = 0; j < value[i].size(); ++j)
            {
                (*data_)[rows_[i]][cols_[j]] = std::to_string(value[i][j]);
            }
        }
    }

    void assign(const std::vector<std::string>& value)
    {
        if (!((rows_.size() == 1 && cols_.size() == value.size()) || (rows_.size() == value.size() && cols_.size() == 1)))
            throw std::runtime_error("operator=() failed. data size is not 1");

        if (rows_.size() == 1 && cols_.size() == value.size())
        {
            for (auto i = 0; i < value.size(); ++i)
            {
                (*data_)[rows_[0]][cols_[i]] = value[i];
            }
        }
        else
        {
            for (auto i = 0; i < value.size(); ++i)
            {
                (*data_)[rows_[i]][cols_[0]] = value[i];
            }
        }
    }

    static std::vector<std::string> split(const std::string& origin, const std::string& separator, const bool& auto_trim=false)
    {
        if (origin.empty())
            return {};
        if (separator.empty())
            return {origin};
        
        std::vector<std::string> result;
        std::size_t separator_size = separator.size();
        std::size_t find_start = 0;

        while (true)
        {
            std::size_t find_position = origin.find(separator, find_start);
            if (find_position == std::string::npos)
            {
                auto elem = std::string(origin.begin() + find_start, origin.end());
                if(auto_trim)
                    result.emplace_back(trim(elem));
                else
                    result.emplace_back(elem);
                break;
            }
            auto elem = std::string(origin.begin() + find_start, origin.begin() + find_position);
            if(auto_trim)
                result.emplace_back(trim(elem));
            else
                result.emplace_back(elem);
            find_start = find_position + separator_size;
        }
        return result;
    }

    static std::string trim(const std::string& origin)
    {
        std::string result = origin;
        const char *whitespaces = " \t\n\r\f\v";
        auto last_current_pos = result.find_last_not_of(whitespaces);
        if (last_current_pos == std::string::npos)
        {
            result.clear();
            return result;
        }
        result.erase(last_current_pos + 1);
        result.erase(0, result.find_first_not_of(whitespaces));
        return result;
    }

};

#endif