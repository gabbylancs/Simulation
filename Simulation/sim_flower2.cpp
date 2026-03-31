//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <string>
//#include <iomanip>
//#include <cmath>
//
//int main()
//{
//    cv::Mat flowerMap = cv::imread("flower_pipe.jpg");
//
//    if (flowerMap.empty())
//    {
//        std::cout << "Error: flower_pipe.jpg not found!" << std::endl;
//        return -1;
//    }
//
//    // --- 1. Pipe & Camera Parameters ---
//    const double pipeRadius = 150.0;  // mm
//    const double pipeLength = 2000.0; // mm (Texture repeat length)
//    const double fx = 600.0, fy = 600.0, cx = 320.0, cy = 240.0;
//
//    // Lens Distortion: k1 > 0 for Barrel, k1 < 0 for Pincushion
//    const double k1 = 0.2;
//
//    // Simulation Settings
//    const int totalFrames = 100;
//    const double speed = 15.0; // mm per frame
//
//    for (int frame = 0; frame < totalFrames; frame++)
//    {
//        // GLOBAL FRAME: Camera starts at Z=0 and moves into the pipe
//        double cameraGlobalZ = frame * speed;
//
//        cv::Mat cameraView = cv::Mat::zeros(480, 640, CV_8UC3);
//        cv::Mat lightMask = cv::Mat::zeros(cameraView.size(), CV_32F);
//
//        for (int v = 0; v < cameraView.rows; v++)
//        {
//            for (int u = 0; u < cameraView.cols; u++)
//            {
//
//                // --- STEP A: Radial Undistortion (Apply Lens Effect) ---
//                // We calculate how the lens "bends" the light before it hits the sensor
//                double dx = (u - cx) / fx;
//                double dy = (v - cy) / fy;
//                double r2 = dx * dx + dy * dy;
//
//                // Barrel distortion formula: x_distorted = x(1 + k1*r^2)
//                double distort = 1.0 + k1 * r2;
//                double normX = dx / distort;
//                double normY = dy / distort;
//
//                // --- STEP B: Cylinder Intersection (Local Frame) ---
//                // t is the distance from the camera to the wall along the ray
//                double t = pipeRadius / std::sqrt(normX * normX + normY * normY);
//                double localZ = t; // Because ray direction Z component is 1.0
//
//                // --- STEP C: Global Mapping ---
//                // The absolute Z position in the pipe
//                double globalPointZ = cameraGlobalZ + localZ;
//
//                double theta = std::atan2(normY, normX);
//                if (theta < 0) theta += 2.0 * CV_PI;
//
//                // Map to texture coordinates with wrap-around
//                int srcX = static_cast<int>((theta / (2.0 * CV_PI)) * flowerMap.cols);
//                int srcY = static_cast<int>(std::fmod(globalPointZ, pipeLength) / pipeLength * flowerMap.rows);
//
//                // Sample texture
//                if (srcX >= 0 && srcX < flowerMap.cols && srcY >= 0 && srcY < flowerMap.rows)
//                {
//                    cameraView.at<cv::Vec3b>(v, u) = flowerMap.at<cv::Vec3b>(srcY, srcX);
//                }
//
//                // --- STEP D: Lighting (Always relative to camera distance) ---
//                double attenuation = 700.0 / (localZ + 200.0);
//                lightMask.at<float>(v, u) = std::fmin(1.0f, std::fmax(0.1f, (float)attenuation));
//            }
//        }
//
//        // --- 2. Post-Processing ---
//
//        // Apply Lighting
//        cv::Mat cameraViewFloat;
//        cameraView.convertTo(cameraViewFloat, CV_32FC3);
//        std::vector<cv::Mat> channels;
//        cv::split(cameraViewFloat, channels);
//        for (int i = 0; i < 3; i++) channels[i] = channels[i].mul(lightMask);
//        cv::merge(channels, cameraViewFloat);
//        cameraViewFloat.convertTo(cameraView, CV_8UC3);
//
//        // Add Sensor Noise
//        cv::Mat noise = cv::Mat::zeros(cameraView.size(), cameraView.type());
//        cv::randn(noise, cv::Scalar(0, 0, 0), cv::Scalar(10, 10, 10));
//        cameraView += noise;
//
//        // --- 3. Save & Display ---
//        std::stringstream ss;
//        ss << "flower_pipe_sims//pipe_frame_" << std::setfill('0') << std::setw(4) << frame << ".jpg";
//        cv::imwrite(ss.str(), cameraView);
//
//        cv::imshow("Global Frame Pipe Simulator", cameraView);
//        if (cv::waitKey(1) == 27) break; // ESC to quit
//    }
//
//    return 0;
//}