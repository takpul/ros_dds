#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <map>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/node/node.hpp>
#include <rclcpp/publisher/publisher.hpp>
#include <future>
#include <memory>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>


#include <genidlcpp/resolver.h>

#include <ccpp_dds_dcps.h>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>

namespace rclcpp
{
    namespace client
    {
        template <typename ROSRequestType, typename ROSResponseType>
        class Client
        {
        public:
            typedef std::shared_ptr<const ROSRequestType> req_shared_ptr;
            typedef std::shared_ptr<const ROSResponseType> res_shared_ptr;
            typedef std::shared_ptr< std::promise<const ROSResponseType&> > shared_promise;
            typedef std::shared_ptr< Client<ROSRequestType, ROSResponseType> > Ptr;
            typedef std::shared_future<const ROSResponseType&> shared_future;


            Client(const std::string& client_id, typename rclcpp::publisher::Publisher<ROSRequestType>::Ptr publisher) : client_id_(client_id), publisher_(publisher), req_id_(0) {}
            ~Client() {}

            void handle_response(const ROSResponseType& res) {
                std::cout << "Got response" << std::endl;
                shared_promise call_promise = this->pending_calls_[res.req_id];
                this->pending_calls_.erase(res.req_id);
                call_promise->set_value(res);
            }

            ROSResponseType call(ROSRequestType &req) {
                return this->async_call(req).get();
            }

            shared_future async_call(ROSRequestType &req) {
                req.req_id = ++(this->req_id_);
                req.client_id = client_id_;

                shared_promise call_promise(new std::promise<const ROSResponseType&>);
                pending_calls_[req.req_id] = call_promise;

                this->publisher_->publish(req);

                return shared_future(call_promise->get_future());
            }

        private:
            typename rclcpp::publisher::Publisher<ROSRequestType>::Ptr publisher_;
            std::map<int, shared_promise> pending_calls_;
            std::string client_id_;
            int req_id_;
        };
    }
}
#endif