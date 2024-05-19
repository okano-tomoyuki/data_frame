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

#ifndef _UTILITY_DATA_FRAME_HPP_
#define _UTILITY_DATA_FRAME_HPP_

#include <vector>               // std::vector
#include <string>               // std::string, std::getline
#include <fstream>              // std::ifstream, std::ofstream
#include <sstream>              // std::sstream, std::istringstream
#include <stdexcept>            // std::runtime_error, std::out_of_range
#include <iostream>             // std::cout, std::endl
#include <algorithm>            // std::find
#include <initializer_list>     // std::initilizer_list
#include <utility>              // std::tuple
#include <unordered_map>

/**
 * @class DataFrame
 * @brief Pythonにおける表形式データハンドリング用ライブラリPandasの代替ライブラリ
 * @note 本家リンクは下記参照のこと
 * @n @link
 * https://pandas.pydata.org/pandas-docs/stable/reference/index.html
 * @endlink
 * @n 現状は Factory Method @ref read_csv からのみインスタンス化可能にしてCSVデータ読込にのみ特化させている。
 * @n 本家と異なり各列の型情報を保持するようにはしていない。各行をtuple化したvectorコンテナとする実装も考えたが、現状は対応させていない。
 * @n また、基本的には静的データの解析に用いることを前提で本クラスは作成しており、全行データをDataFrameとして取り込んだ後、
 * @n 加工して使用することを想定している。高速な読取処理については今後も本クラスで対応する予定はないため要望に応じて別クラスを作成する。
 *
 */
class DataFrame final
{

public:
    enum Axis
    { 
        COLUMN, 
        ROW    
    };

    enum ReadCsvArgument
    {
        HEADER,
        SEPARATOR,
        NEW_LINE,
        AUTO_TRIM
    };

    class DynamicType
    {
    private:
        enum struct Kind { BOOLEAN, NUMBER, STRING };

        union Data
        {
            bool boolean;
            double number;
            std::string str;
            Data() : boolean() {}
            ~Data() {}
        };

        Kind kind_;
        Data data_;

        template<typename T>
        void destroy(T* t)
        {
            t->~T();
        }

        template<class T, class = void>
        struct DynamicAs
        {
            static T as(const DynamicType& value)
            {
                T result;
                std::stringstream ss;
                if(value.kind_ == Kind::BOOLEAN)
                    ss << value.data_.boolean;
                else if(value.kind_ == Kind::NUMBER)
                    ss << value.data_.number;
                ss >> result;
                return result;
            }
        };

        template<class V>
        struct DynamicAs<std::string, V>
        {
            static std::string as(const DynamicType& value)
            {
                return value.data_.str;
            }
        };

    public:
        DynamicType()
        : kind_()
        {}

        DynamicType(const DynamicType& other)
        : kind_(other.kind_)
        {
            if(kind_ == Kind::BOOLEAN)
                data_.boolean = other.data_.boolean;
            else if(kind_ == Kind::NUMBER)
                data_.number = other.data_.number;
            else if(kind_ == Kind::STRING)
                new(&data_.str) std::string(other.data_.str);
        }

        DynamicType(const bool& value)
        : kind_(Kind::BOOLEAN)
        {
            data_.boolean = value;
        }

        DynamicType(const double& value)
        : kind_(Kind::NUMBER)
        {
            data_.number = value;
        }

        DynamicType(const char* value)
        : kind_(Kind::STRING)
        {
            new(&data_.str) std::string(value);
        }

        DynamicType(const std::string& value)
        : kind_(Kind::STRING)
        {
            new(&data_.str) std::string(value);
        }

        template<typename T>
        T as() const
        {
            return DynamicAs<T>::as(*this);
        }

        ~DynamicType()
        {
            if(kind_ == Kind::STRING)
                destroy(&data_.str);
        }
    };

    /**
     * @fn operator=
     * @brief コピーメソッド
     * 
     * @param DataFrame  
     */
    void operator=(const DataFrame& other)
    {
        header_ = other.header_;
        data_   = other.data_;
    }

    /**
     * @fn read_csv
     * @brief CSV読取メソッド (Factory Method)
     * 
     * @param std::string file_path csvのファイルパス
     * @param 
     * @return DataFrame 読取後DataFrameインスタンス
     */
    static DataFrame read_csv(const std::string& file_path, const std::unordered_map<ReadCsvArgument, DynamicType>& arg_map)
    {
        const auto header       = arg_map.count(HEADER)    ? arg_map.at(HEADER).as<bool>()           : true;
        const auto separator    = arg_map.count(SEPARATOR) ? arg_map.at(SEPARATOR).as<std::string>()  : ",";
        const auto new_line     = arg_map.count(NEW_LINE)  ? arg_map.at(NEW_LINE).as<std::string>()  : "\n";
        const auto auto_trim    = arg_map.count(AUTO_TRIM) ? arg_map.at(AUTO_TRIM).as<bool>()        : true;
        return read_csv(file_path, header, separator, new_line, auto_trim);
    }

    /**
     * @fn read_csv
     * @brief CSV読取メソッド (Factory Method)
     * 
     * @param std::string file_path csvのファイルパス
     * @param 
     * @return DataFrame 読取後DataFrameインスタンス
     */
#ifdef __unix__
    static DataFrame read_csv(const std::string& file_path, const bool& header=true, const std::string& separator = ",", const std::string& new_line =   "\n", const bool& auto_trim = true)
#else
    static DataFrame read_csv(const std::string& file_path, const bool& header=true, const std::string& separator = ",", const std::string& new_line = "\r\n", const bool& auto_trim = true)
#endif
    {
        std::vector<std::string> header_row;
        std::vector<std::vector<std::string>> data;

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
        if(header)
        {
            header_row = split(line_list.front(), separator, auto_trim);
            line_list.erase(line_list.begin());
        }
        else
        {
            for(auto i=0; i < split(line_list.front(), separator, auto_trim).size();i++) 
                header_row.push_back(std::to_string(i));
        }

        int row_index = 0;
        data.reserve(line_list.size());
        for(auto&& line : line_list)
        {
            auto row = split(line, separator, auto_trim);
            if(row.size() != header_row.size())
            {
                std::stringstream ss;
                ss  << "line[" << row_index + static_cast<int>(header) << "] element size between header and row is different."
                    << "header's element size : " << header_row.size() << "row's element size : " << row.size();  
                throw std::runtime_error(ss.str());
            }
            data.push_back(row);
            row_index++;
        }

        return DataFrame(header_row, std::move(data));
    }

    /**
     * @fn to_csv
     * @brief csvファイルへの書込メソッド
     * 
     * @param std::stirng file_path 書込先ファイルパス 
     * @param append 追記モードか、上書きモードか
     * @param header ヘッダーを出力データに含めるか
     * @param separator 区切り文字
     */
    void to_csv(const std::string& file_path, const bool& append=false, const bool& header=true, const std::string& separator=",") const
    {
        std::ofstream ofs;
        append ? ofs.open(file_path, std::ios::app) : ofs.open(file_path); // switching append or overwrite.
        if(!ofs) 
            throw std::runtime_error("file path '" + file_path + "' doesn't exist.");

        if(header)
            ofs << concat(header_, separator) << std::endl;
        
        std::stringstream ss;
        for (const auto& row : data_)
            ss << concat(row, separator) << std::endl;
        
        auto result = ss.str();
        
        result.pop_back(); // pop back latest new line
        ofs << result;
    }


    /**
     * @fn operator[]
     * @brief 列名によるDataFrameの切出メソッド
     * 
     * @param std::stirng target_column 取得対象の列名
     * @return DataFrame 切出処理後の新たなDataFrameインスタンス
     */
    DataFrame operator[](const std::string& target_column) const
    {
        auto itr = std::find(header_.begin(), header_.end(), target_column);
        
        if (itr==header_.end())
        {
            std::stringstream ss;
            ss << "target column '" << target_column << "' was not found.";
            throw std::runtime_error(ss.str());
        }

        int index = std::distance(header_.begin(), itr);
        std::vector<std::string> header = {header_.at(index)};
        std::vector<std::vector<std::string>> data;

        for(const auto& row : data_)
            data.push_back({row.at(index)});

        return DataFrame(header, std::move(data));
    }

    /**
     * @fn operator[]
     * @brief 列名によるDataFrameの切出メソッド
     * 
     * @param std::stirng target_column 取得対象の列名
     * @return DataFrame 切出処理後の新たなDataFrameインスタンス
     */
    DataFrame operator[](const char* target_column) const
    {
        return this->operator[](std::string(target_column));
    }

    /**
     * @fn operator[]
     * @brief 列名によるDataFrameの切出メソッド
     * 
     * @param std::vector<std::stirng> target_column_list 取得対象の列名のリスト
     * @return DataFrame 切出処理後の新たなDataFrameインスタンス
     */
    DataFrame operator[](const std::vector<std::string>& target_column_list) const
    {
        std::vector<int> indices;
        for (const auto& column : target_column_list)
        {
            auto itr = std::find(header_.begin(), header_.end(), column);
            if (itr==header_.end())
                throw std::runtime_error("target column was not found.");
            int index = std::distance(header_.begin(), itr);
            indices.push_back(index);
        }

        std::vector<std::string> header;
        for(const auto& index : indices)
            header.push_back(header_[index]);

        std::vector<std::string> row_data;
        std::vector<std::vector<std::string>> data;
        for(const auto& row : data_)
        {
            row_data.clear();
            for(const auto& index : indices)
                row_data.push_back(row[index]);
            data.push_back(row_data);
        }

        return DataFrame(header, std::move(data));
    }

    /**
     * @fn operator[]
     * @brief 行インデックスによるDataFrameの切出メソッド
     * 
     * @param std::stirng target_row 取得対象の行インデックス
     * @return DataFrame 切出処理後の新たなDataFrameインスタンス
     * @note 負数を指定した場合の取得対象行のインデックスはpandasの仕様に従う。
     * @n    --例--  df[-1] == df[0] ... true
     */
    DataFrame operator[](const int& target_row) const
    {
        int index;
        index = target_row >= 0 ? target_row : data_.size() + target_row;
        if (index < 0 || index >= data_.size())
            throw std::out_of_range("index number [" + std::to_string(target_row) + "] was out of range");

        return DataFrame(header_, {data_[index]});
    }

    /**
     * @fn slice
     * @brief 行インデックスの開始・終了指定によるDataFrameの切出メソッド
     * 
     * @param int start_index 開始インデックス
     * @param int end_index   終了インデックス 
     * @return DataFrame 切出処理後の新たなDataFrameインスタンス
     */
    DataFrame slice(const int& start_index, const int& end_index) const
    {
        const int s_index = (start_index  >= 0) ? start_index  : data_.size() + start_index;
        const int e_index = (end_index    >= 0) ? end_index    : data_.size() + end_index;
        if (s_index < 0 || s_index >= data_.size())
            throw std::out_of_range("start index number was out of range");
        if (e_index   < 0 || e_index   >= data_.size())
            throw std::out_of_range("end index number was out of range");
        if (s_index > e_index)
            throw std::out_of_range("end index must be larger than start index.");
        
        std::vector<std::vector<std::string>> data;
        for(auto i = s_index; i < e_index; i++)
            data.push_back(data_[i]);

        return DataFrame(header_, std::move(data));
    }

    /**
     * @fn rename
     * @brief 列名のリネームメソッド
     * 
     * @param std::initializer_list<std::stirng> header リネーム後のヘッダー名のリスト 
     * @return DataFrame& リネーム後の自身のインスタンス
     */
    DataFrame& rename(const std::initializer_list<std::string>& header)
    {
        std::vector<std::string> v(header);
        this->rename(v);
        return *this;
    }

    /**
     * @fn rename
     * @brief 列名のリネームメソッド
     * 
     * @param std::vector<std::stirng> header リネーム後のヘッダー名のリスト 
     * @return DataFrame& リネーム後の自身のインスタンス
     */
    DataFrame& rename(const std::vector<std::string> header)
    {
        if(header.size()!=header_.size())
            throw std::runtime_error("header size is different");
        header_ = header;
        return *this;
    }

    /**
     * @fn describe
     * @brief メタ情報取得表示メソッド
     */ 
    void describe() const
    {
        std::cout << "header names: {" << concat(header_, ",") << "}" << std::endl;
        std::cout << "    row size: " << data_.size() << std::endl;
        std::cout << " column size: " << header_.size() << std::endl;
    }

    /**
     * @fn data
     * @brief データ取り出しメソッド
     *  
     * @return std::vector<std::vector<std::string> data 
     */
    std::vector<std::vector<std::string>> data() const
    {
        return data_;
    }

    /**
     * @fn to_matrix
     * @brief 2次元ベクターに変換するメソッド
     * 
     * @tparam T 
     * @return std::vector<T> 
     */
    template<typename T>
    std::vector<std::vector<T>> to_matrix() const
    {
        std::vector<std::vector<T>> result;
        std::vector<T> tmp;
        result.reserve(data_.size());
        for(const auto& line : data_)
        {
            tmp.clear();
            tmp.reserve(line.size());
            for(const auto& e : line)
            {
                tmp.push_back(As<T>::as(e));
            }
            result.push_back(tmp);
        }
        return result;
    }

    /**
     * @fn to_vector
     * @brief 列・行いずれかを指定の型の1次元ベクターに変換するメソッド
     * 
     * @tparam T 
     * @param enum Axis axis ベクター化したい軸 { ROW : 行, COLUMN : 列 } 
     * @return std::vector<T> 
     */
    template<typename T>
    std::vector<T> to_vector(const enum Axis& axis=COLUMN) const
    {
        if(axis==ROW && data_.size() != 1)
            throw std::runtime_error("to_vector method can be used to 1 raw DataFrame only.");
        if(axis==COLUMN && header_.size() != 1)
            throw std::runtime_error("to_vector method can be used to 1 column DataFrame only.");
        
        std::vector<T> result;
        if(axis==COLUMN)
            for(const auto& row : data_)
                result.push_back(As<T>::as(row[0]));
        else // ROW
            for(const auto& e : data_[0])
                result.push_back(As<T>::as(e));
        return result;
    } 

    /**
     * @fn as
     * @brief 1行・1列のDataFrameインスタンスを特定の型に変換する
     * 
     * @tparam T 
     * @return T 取得したい型に変換したデータ返却する
     */
    template<typename T>
    T as() const
    {
        if(data_.size() != 1 || data_.at(0).size() != 1)
        {
            throw std::runtime_error("as method can be used to 1 raw and 1 column DataFrame only.");
        }

        return As<T>::as(data_[0][0]);
    }

    operator int() const
    {
        return as<int>();
    }

    operator double() const
    {
        return as<double>();
    }

    operator std::string() const
    {
        return as<std::string>();
    }

    operator std::vector<int>() const
    {
        if(data_.size() == 1)
            return to_vector<int>(ROW);
        else
            return to_vector<int>(COLUMN);
    }

    operator std::vector<double>() const
    {
        if(data_.size() == 1)
            return to_vector<double>(ROW);
        else
            return to_vector<double>(COLUMN);
    }

    operator std::vector<std::string>() const
    {
        if(data_.size() == 1)
            return to_vector<std::string>(ROW);
        else
            return to_vector<std::string>(COLUMN);
    }

    operator std::vector<std::vector<int>>() const
    {
        return to_matrix<int>();
    }

    operator std::vector<std::vector<double>>() const
    {
        return to_matrix<double>();
    }

    operator std::vector<std::vector<std::string>>() const
    {
        return to_matrix<std::string>();
    }

private:
    std::vector<std::string>  header_;
    std::vector<std::vector<std::string>> data_;

    static std::string concat(const std::vector<std::string>& origin, const std::string& separator)
    {
        std::string result;
        for (const auto& str : origin)
            result += str + separator;
        
        auto separator_size = separator.size();
        while(separator_size--)
            result.pop_back();
        return result;
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

    template<class T, class = void> 
    struct As 
    {
        static T as(const std::string& value) 
        {
            T result;
            std::stringstream ss;
            ss << value;        
            ss >> result;
            return result;
        }
    };

    template<class V> 
    struct As<std::string, V> 
    {
        static std::string as(const std::string& value) 
        {
            return value;
        }
    }; 

    explicit DataFrame(const std::vector<std::string>& header, const std::vector<std::vector<std::string>>&& data)
     : header_(header), data_(std::move(data))
    {}
};

#endif