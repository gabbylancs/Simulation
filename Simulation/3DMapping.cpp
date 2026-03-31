//#include <Eigen/Dense>           // 1. MUST BE FIRST
//#include <opencv2/opencv.hpp>    // 2. STANDARD OPENCV
//#include <opencv2/core/eigen.hpp>// 3. THE BRIDGE
//#include <iostream>
//#include <vector>
//#include <map>
//#include <algorithm>
//#include <cmath>
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