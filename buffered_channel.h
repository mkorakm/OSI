#ifndef BUFFERED_CHANNEL_H
#define BUFFERED_CHANNEL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <utility> 

template<class T>
class BufferedChannel {
private:
    int buffer_size_;         
    std::queue<T> queue_;               
    std::mutex mtx_;                   
    std::condition_variable not_full_;  
    std::condition_variable not_empty_; 
    bool is_closed_;                    

public:

    explicit BufferedChannel(int size) : buffer_size_(size), is_closed_(false) {
        if (size == 0) {
            throw std::invalid_argument("Buffer size must be greater than 0");
        }
    }

    BufferedChannel(const BufferedChannel&) = delete;
    BufferedChannel& operator=(const BufferedChannel&) = delete;

    void send(T value) {
        std::unique_lock<std::mutex> lock(mtx_);

        not_full_.wait(lock, [this]() {
            return queue_.size() < static_cast<size_t>(buffer_size_) || is_closed_;
        });

        if (is_closed_) {
            throw std::runtime_error("Channel is closed");
        }

        queue_.push(std::move(value));

        not_empty_.notify_one();
    }

    std::pair<T, bool> recv() {
        std::unique_lock<std::mutex> lock(mtx_);

        not_empty_.wait(lock, [this]() {
            return !queue_.empty() || is_closed_;
            });

        if (queue_.empty() && is_closed_) {
            return { T(), false };
        }

        T value = std::move(queue_.front());
        queue_.pop();

       
        not_full_.notify_one();

        return { std::move(value), true };
    }

 
    void close() {
        std::unique_lock<std::mutex> lock(mtx_);

        is_closed_ = true;

        not_full_.notify_all();
        not_empty_.notify_all();
    }
};


#endif // BUFFERED_CHANNEL_H
