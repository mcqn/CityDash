require 'mqtt'
require 'json'
require 'base64'
require 'yaml'
require 'net/https'

# CityDash message globals
kCityDashStatusOk = 0.chr
kCityDashStatusPending = 1.chr
kCityDashStatusFault = 2.chr

# Read in config
settings = nil
if ARGV.length == 1
  settings = YAML.load_file(ARGV[0])
else
  puts "No configuration provided."
  exit
end

def publish_ttn_message(mqtt_client, app_id, dev_id, payload, ttl)
  # Need to base64 encode the payload
  # Build up parameters for the TTN downlink
  params = {
    "payload" => Base64.encode64(payload).strip,
    "ttl" => ttl,
    "port" => 1
  }
  # Send the message
puts "#{app_id}/devices/#{dev_id}/down"
puts params.to_json
  mqtt_client.publish("#{app_id}/devices/#{dev_id}/down", params.to_json)
end


MQTT::Client.connect("mqtts://#{settings['ttn']['user']}:#{settings['ttn']['pass']}@staging.thethingsnetwork.org") do |c|
  c.get('#') do |topic, message|
    puts "#{topic}: #{message}"
    if topic.match(/up$/)
      # We've got a message we care about
      data = JSON.parse(message)
      puts data['payload']
      devID = data['dev_eui']
      status = Base64.decode64(data['payload'])

      # Find a github issue with that tag
      url_params = "?state=open&labels=#{devID}"
      if (settings["issues"]["url_is_https"])
        issue_uri = URI.parse(settings["issues"]["url"]+url_params)
        issue_http = Net::HTTP.new(issue_uri.host, issue_uri.port)
        issue_http.use_ssl = true
        issue_req = Net::HTTP::Get.new(issue_uri.request_uri, {'User-Agent' => "CityDash/1.0"})
        issue_data = issue_http.request(issue_req)
      else
        issue_data = Net::HTTP.get_response(URI.parse(settings["issues"]["url"]+url_params))
      end
      issues = JSON.parse(issue_data.body)

      if status == kCityDashStatusPending
        puts "#{devID} button pressed"

        if issues.empty?
          # New reported fault
          puts "No open issue for #{devID}"
          # Create new issue
          new_issue_params = {
            "title" => "#{devID} issue logged",
            "labels" => [devID]
          }
          if (settings["issues"]["url_is_https"])
            issue_uri = URI.parse(settings["issues"]["url"]+"?access_token=#{settings['issues']['oauth_token']}")
            issue_http = Net::HTTP.new(issue_uri.host, issue_uri.port)
            issue_http.use_ssl = true
            issue_req = Net::HTTP::Post.new(issue_uri.request_uri, {'User-Agent' => "CityDash/1.0"})
            issue_req.body = "#{new_issue_params.to_json}"
            issue_data = issue_http.request(issue_req)
          else
            issue_data = Net::HTTP.post_response(URI.parse(settings["issues"]["url"]+url_params))
          end
          if issue_data.code.to_i >= 200 && issue_data.code.to_i < 300
            # Issue created okay, mark it as a fault
            publish_ttn_message(c, settings['ttn']['user'], devID, kCityDashStatusFault, "1h")
          else
            puts "Problem creating issue: #{issue_data.code} #{issue_data.message}: #{issue_data.body}"
          end
        else
          # It's already been reported
          puts "Already an issue #{issues[0]}"
          publish_ttn_message(c, settings['ttn']['user'], devID, kCityDashStatusFault, "1h")
          # FIXME Add a nudge comment
        end
      else
        if issues.empty?
          # Either the device thinks everything's okay and it is, or
          # the device thinks there's a fault but it's been cleared
          publish_ttn_message(c, settings['ttn']['user'], devID, kCityDashStatusOk, "1h")
        else
          # Either the device thinks everything's fine but it isn't, or
          # the device knows there's a fault and it hasn't been fixed
          publish_ttn_message(c, settings['ttn']['user'], devID, kCityDashStatusFault, "1h")
        end
      end
    end
  end
end

