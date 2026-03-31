#include <Eigen/Dense>           // 1. MUST BE FIRST
#include <opencv2/opencv.hpp>    // 2. STANDARD OPENCV
#include <opencv2/core/eigen.hpp>// 3. THE BRIDGE
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <fstream> // Add this at the top with other includes

int main()
{
    //const double pipeRadius = 67.42/2; //mm
    const double pipeRadius = 150;
    //const double fx = 2608.721575, fy = 2584.771277, cx = 1493.821507, cy = 1191.874317;
    const double fx = 600.0, fy = 600.0, cx = 320.0, cy = 240.0;

    //cv::Mat img1 = cv::imread("flower_pipe_sims_1/pipe_frame_0001.jpg");
    //cv::Mat img2 = cv::imread("flower_pipe_sims_1/pipe_frame_0007.jpg");

    cv::Mat img1 = cv::imread("pipe_frame_0090.jpg");
    cv::Mat img2 = cv::imread("pipe_frame_0095.jpg");


    //cv::Mat img1 = cv::imread("images/frame1.jpg");
    //cv::Mat img2 = cv::imread("images/frame2.jpg");


    if (img1.empty() || img2.empty()) return -1;

    // --- 1. PREPROCESSING ---
    auto preprocess = [](cv::Mat& img) {
        cv::Mat gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

        // CLAHE to handle lighting in dark pipes
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        clahe->apply(gray, gray);

        // Slight Blur to reduce sensor noise
        cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);
        return gray;
        };

    cv::Mat gray1 = preprocess(img1);
    cv::Mat gray2 = preprocess(img2);

    // --- 2. FEATURE DETECTION ---
    cv::Ptr<cv::ORB> orb = cv::ORB::create(2000); // Increased for better RANSAC pool
    std::vector<cv::KeyPoint> kp1, kp2;
    cv::Mat desc1, desc2;
    orb->detectAndCompute(gray1, cv::noArray(), kp1, desc1);
    orb->detectAndCompute(gray2, cv::noArray(), kp2, desc2);

    // --- 3. MATCHING ---
    cv::BFMatcher matcher(cv::NORM_HAMMING);
    std::vector<cv::DMatch> matches;
    matcher.match(desc1, desc2, matches);

    // --- 4. RANSAC FILTERING ---
    std::vector<cv::Point2f> pts1, pts2;
    for (const auto& m : matches) {
        pts1.push_back(kp1[m.queryIdx].pt);
        pts2.push_back(kp2[m.trainIdx].pt);
    }

    // Find Fundamental Matrix with RANSAC to identify inliers
    std::vector<uchar> inliersMask;
    cv::findFundamentalMat(pts1, pts2, cv::FM_RANSAC, 3.0, 0.99, inliersMask);

    std::vector<cv::DMatch> ransacMatches;
    for (size_t i = 0; i < inliersMask.size(); i++) {
        if (inliersMask[i]) {
            ransacMatches.push_back(matches[i]);
        }
    }

    cv::Mat inliersMask2;
    cv::Mat K = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
    cv::Mat E = cv::findEssentialMat(pts1, pts2, K, cv::RANSAC, 0.999, 1.0, inliersMask2);
    cv::Mat R, t;
    cv::recoverPose(E, pts1, pts2, K, R, t, inliersMask2);

    // --- NEW: ESTIMATE DISTANCE MOVED USING PIPE GEOMETRY ---
    double estimatedDistance = 0.0;
    std::vector<cv::Point3f> pointCloud;

	int featureCount = 0;
    if (!t.empty() && !ransacMatches.empty()) 
    {
        // 1. Back-project rays from Frame 1 onto the cylinder wall
        // A point on the cylinder (centered at 0,0) satisfies X^2 + Y^2 = R^2
        std::vector<cv::Point2f> inliers1, inliers2;
        std::vector<cv::Point3f> pts3D_frame1;

        for (size_t i = 0; i < inliersMask2.rows; i++) {
            if (inliersMask2.at<uchar>(i)) 
            {
				featureCount++;
                cv::Point2f p = pts1[i];
                // ray direction
                double rx = (p.x - cx) / fx;
                double ry = (p.y - cy) / fy;
                double rz = 1.0;

                // Intersection of ray s*[rx, ry, rz] with X^2 + Y^2 = R^2
                // s^2 * (rx^2 + ry^2) = R^2
                double s = pipeRadius / std::sqrt(rx * rx + ry * ry);
                pts3D_frame1.push_back(cv::Point3f(s * rx, s * ry, s * rz));

                inliers1.push_back(pts1[i]);
                inliers2.push_back(pts2[i]);
            }
        }

        // 2. Triangulate with unit t to find the relative scale
        cv::Mat P1 = K * cv::Mat::eye(3, 4, CV_64F);
        cv::Mat Rt2_unit;
        cv::hconcat(R, t, Rt2_unit);
        cv::Mat P2 = K * Rt2_unit;

        cv::Mat pts4D;
        cv::triangulatePoints(P1, P2, inliers1, inliers2, pts4D);

        // 3. Compare Triangulated Depth vs Cylindrical Depth to find scale
        double scaleSum = 0;
        int count = 0;
        for (int i = 0; i < pts4D.cols; i++) {
            float w = pts4D.at<float>(3, i);
            cv::Point3f p_tri(pts4D.at<float>(0, i) / w, pts4D.at<float>(1, i) / w, pts4D.at<float>(2, i) / w);

            if (p_tri.z > 0) {
                // Scale = Real Depth / Unit Depth
                double real_z = pts3D_frame1[i].z;
                scaleSum += (real_z / p_tri.z);
                count++;
            }
        }

        if (count > 0) {
            double finalScale = scaleSum / count;
            cv::Mat t_metric = t * finalScale;
            estimatedDistance = cv::norm(t_metric); // Distance in mm

            // Populate point cloud for mapping
            for (auto& p : pts3D_frame1) pointCloud.push_back(p);
        }
    }

    // --- RECORD INITIAL ESTIMATES TO FILE ---
    std::ofstream outFile("initial_estimates.txt");
    if (outFile.is_open()) {
        outFile << "--- CAMERA INITIAL GUESSES ---\n";
        outFile << "xc_mm: " << 0 << "\n";
        outFile << "yc_mm: " << 0 << "\n";
        outFile << "delta_z_mm: " << estimatedDistance << "\n\n";

        outFile << "--- FEATURE INITIAL GUESSES (theta, z) ---\n";
        outFile << "count: " << featureCount << "\n";
        outFile << "index, theta_rad, z_mm\n";

        /*for (size_t i = 0; i < featureCount; ++i) 
        {
            outFile << i << ", "
                << initialFeatures[i].theta << ", "
                << initialFeatures[i].z << "\n";
        }*/
        outFile.close();
        std::cout << "Initial estimates successfully saved to initial_estimates.txt" << std::endl;
    }
    else 
    {
        std::cerr << "Error: Could not open file for writing." << std::endl;
    }

    // --- 5. VISUALIZATION ---
    cv::Mat imgMatches;
    cv::hconcat(img1, img2, imgMatches);

    for (const auto& m : ransacMatches)
    {
        cv::Point2f pt1 = kp1[m.queryIdx].pt;
        cv::Point2f pt2 = kp2[m.trainIdx].pt;
        cv::Point2f pt2_shifted = pt2 + cv::Point2f((float)img1.cols, 0.0f);

        cv::line(imgMatches, pt1, pt2_shifted, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
    }

    // --- NEW: DRAW EPIPOLE ---
    if (!t.empty()) {
        double tx = t.at<double>(0), ty = t.at<double>(1), tz = t.at<double>(2);
        if (std::abs(tz) > 0.01) { // Forward motion check
            cv::Point2f epipole(static_cast<float>((fx * tx / tz) + cx),
                static_cast<float>((fy * ty / tz) + cy));

            cv::Point2f epShifted = epipole + cv::Point2f((float)img1.cols, 0);
            cv::drawMarker(imgMatches, epShifted, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 40, 3);
        }
    }


    // Resize the final combined image for display (e.g., 50% scale)
    cv::Mat displayImg;
    double scale = 0.8;
    cv::resize(imgMatches, displayImg, cv::Size(), scale, scale, cv::INTER_LINEAR);

    cv::imshow("RANSAC Inlier Matches (Resized for Display)", displayImg);
    cv::waitKey(0);

    return 0;
}
// 
// 
//
//struct Point3D {
//    Eigen::Vector3d pos;
//    int id;
//};
//
//struct GlobalLandmark {
//    Eigen::Vector3d pos;
//    int observations = 0;
//};
//
//// --- GLOBAL MAP WITH LANDMARK PERSISTENCE ---
//struct GlobalMap {
//    std::map<int, GlobalLandmark> landmarks;
//    Eigen::Matrix3d R_world = Eigen::Matrix3d::Identity();
//    Eigen::Vector3d t_world = Eigen::Vector3d::Zero();
//
//    void addFusedPoints(const std::vector<Point3D>& local_points,
//        const cv::Mat& R_curr, const cv::Mat& t_curr,
//        double motor_dist_mm, double alpha)
//    {
//        Eigen::Matrix3d R_step;
//        Eigen::Vector3d t_direction;
//        cv::cv2eigen(R_curr, R_step);
//        cv::cv2eigen(t_curr, t_direction);
//
//        // Ensure forward motion
//        if (t_direction.z() < 0) t_direction = -t_direction;
//
//        // Apply Scale Fusion
//        double fused_scale = motor_dist_mm; // Can be expanded to trust VO more
//
//        // Update Global Pose
//        t_world += R_world * (t_direction * fused_scale);
//        R_world = R_world * R_step;
//
//        // INTEGRATION: This makes features "stick" by using their IDs
//        for (const auto& lp : local_points) {
//            Eigen::Vector3d p_global = R_world * lp.pos + t_world;
//
//            if (landmarks.find(lp.id) != landmarks.end()) {
//                // Feature already exists: Average position to kill jitter/drift
//                auto& gp = landmarks[lp.id];
//                gp.pos = (gp.pos * gp.observations + p_global) / (gp.observations + 1);
//                gp.observations++;
//            }
//            else {
//                // New Landmark: Anchor it
//                landmarks[lp.id] = { p_global, 1 };
//            }
//        }
//        std::cout << "Robot Z: " << t_world.z() << " mm | Map Points: " << landmarks.size() << std::endl;
//    }
//};
//
//// --- RECONSTRUCTION WITH OFFSET ---
//std::vector<Point3D> reconstructPipePoints(const std::vector<cv::Point2f>& pts1, const std::vector<int>& ids,
//    double pipe_radius, Eigen::Vector2d offset, double fx, double fy, double cx, double cy)
//{
//    std::vector<Point3D> reconstructed_points;
//    double ox = offset.x();
//    double oy = offset.y();
//
//    for (size_t i = 0; i < pts1.size(); ++i) {
//        double dx = (pts1[i].x - cx) / fx;
//        double dy = (pts1[i].y - cy) / fy;
//        Eigen::Vector3d ray(dx, dy, 1.0);
//        ray.normalize(); // Normalize for quadratic solver
//
//        // Quadratic: (s*rx - ox)^2 + (s*ry - oy)^2 = R^2
//        double A = ray.x() * ray.x() + ray.y() * ray.y();
//        double B = -2.0 * (ray.x() * ox + ray.y() * oy);
//        double C = (ox * ox + oy * oy) - (pipe_radius * pipe_radius);
//
//        double discriminant = B * B - 4 * A * C;
//        if (discriminant < 0) continue;
//
//        double s = (-B + std::sqrt(discriminant)) / (2.0 * A);
//        reconstructed_points.push_back({ s * ray, ids[i] });
//    }
//    return reconstructed_points;
//}
//
//void visualizeGlobalMap(const GlobalMap& map, double pipeRadius, Eigen::Vector2d offset)
//{
//    cv::Mat canvas = cv::Mat::zeros(400, 1200, CV_8UC3);
//    double scale = 0.4;
//    int offsetX = 100, offsetY = 200;
//
//    // Draw Walls
//    int wallLimit = (int)(pipeRadius * scale);
//    cv::line(canvas, { 0, offsetY - wallLimit }, { 1200, offsetY - wallLimit }, { 0,0,100 }, 2);
//    cv::line(canvas, { 0, offsetY + wallLimit }, { 1200, offsetY + wallLimit }, { 0,0,100 }, 2);
//
//    // Draw Landmarks (Green)
//    for (const auto& pair : map.landmarks) {
//        const auto& p = pair.second.pos;
//        int iz = (int)(p.z() * scale) + offsetX;
//        int ix = (int)(p.x() * scale) + offsetY;
//        if (iz >= 0 && iz < canvas.cols && ix >= 0 && ix < canvas.rows)
//            cv::circle(canvas, { iz, ix }, 1, { 0, 255, 0 }, -1);
//    }
//
//    // Draw Robot (Orange/Blue) - Account for offset in view
//    int camZ = (int)(map.t_world.z() * scale) + offsetX;
//    int camX = (int)((map.t_world.x() + offset.x()) * scale) + offsetY;
//
//    if (camZ >= 0 && camZ < canvas.cols) {
//        cv::circle(canvas, { camZ, camX }, 10, { 255, 150, 0 }, -1);
//        cv::putText(canvas, "ROBOT", { camZ + 15, camX }, 0, 0.5, { 255, 255, 255 }, 1);
//    }
//
//    cv::imshow("Z-X Global Map", canvas);
//    cv::waitKey(1);
//}
//
//int main()
//{
//    const double pipeRadius = 150.0;
//    const double fx = 600.0, fy = 600.0, cx = 320.0, cy = 240.0;
//    Eigen::Vector2d cameraOffset(0.0, 0.0); // Robot can be moved off center
//
//    GlobalMap pipeMap;
//    cv::Ptr<cv::ORB> orb = cv::ORB::create(1000);
//    cv::BFMatcher matcher(cv::NORM_HAMMING);
//
//    for (int f = 0; f < 90; f += 10) {
//        std::string p1 = "flower_pipe_sims/pipe_frame_" + std::to_string(f).insert(0, 4 - std::to_string(f).length(), '0') + ".jpg";
//        std::string p2 = "flower_pipe_sims/pipe_frame_" + std::to_string(f + 10).insert(0, 4 - std::to_string(f + 10).length(), '0') + ".jpg";
//
//        cv::Mat img1 = cv::imread(p1, 0), img2 = cv::imread(p2, 0);
//        if (img1.empty() || img2.empty()) break;
//
//        std::vector<cv::KeyPoint> kp1, kp2;
//        cv::Mat desc1, desc2;
//        orb->detectAndCompute(img1, cv::noArray(), kp1, desc1);
//        orb->detectAndCompute(img2, cv::noArray(), kp2, desc2);
//
//        std::vector<cv::DMatch> matches;
//        matcher.match(desc1, desc2, matches);
//        std::sort(matches.begin(), matches.end());
//        if (matches.size() > 150) matches.erase(matches.begin() + 150, matches.end());
//
//        std::vector<cv::Point2f> pts1, pts2;
//        std::vector<int> ids;
//        for (const auto& m : matches) {
//            pts1.push_back(kp1[m.queryIdx].pt);
//            pts2.push_back(kp2[m.trainIdx].pt);
//            ids.push_back(m.queryIdx); // Keep original ID for stickiness
//        }
//
//        cv::Mat mask, R, t;
//        cv::Mat E = cv::findEssentialMat(pts1, pts2, fx, { cx, cy }, cv::RANSAC, 0.999, 1.0, mask);
//        cv::recoverPose(E, pts1, pts2, R, t, fx, { cx, cy }, mask);
//
//        std::vector<cv::Point2f> inliers1;
//        std::vector<int> inlierIds;
//        for (int i = 0; i < mask.rows; i++) {
//            if (mask.at<uchar>(i)) {
//                inliers1.push_back(pts1[i]);
//                inlierIds.push_back(ids[i]);
//            }
//        }
//
//        auto localPoints = reconstructPipePoints(inliers1, inlierIds, pipeRadius, cameraOffset, fx, fy, cx, cy);
//        pipeMap.addFusedPoints(localPoints, R, t, 150.0, 0.6);
//
//        visualizeGlobalMap(pipeMap, pipeRadius, cameraOffset);
//        if (cv::waitKey(30) == 27) break;
//    }
//    cv::waitKey(0);
//    return 0;
//}